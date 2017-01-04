#include <numeric>
#include <typeindex>
#include <regex>

#include <polysync/descriptor.hpp>
#include <polysync/detector.hpp>
#include <polysync/exception.hpp>
#include <polysync/logging.hpp>
#include <polysync/print_tree.hpp>
#include <polysync/print_hana.hpp>

namespace polysync {

using polysync::logging::logger;
using polysync::logging::severity;

namespace detector {

Catalog catalog;

// Helper function to get an integer out of a hex string.  This would not be
// necessary if TOML supported hex formatted integers natively.
template <typename T>
inline T stoulCast( const std::string& value )
{
    return boost::numeric_cast<T>( std::stoul( value, 0, 0 ) );
}

// Description strings have type information, but the type comes out as a
// string (because TOML does not have a very powerful type system).  Define
// factory functions to look up a type by string name and convert to a strong type.
static variant parseTerminalFromString( const std::string value, const std::type_index& type )
{
    static const std::map< std::type_index,
                           std::function< variant (const std::string&) > > factory =
    {
        { typeid(std::int8_t),
            []( const std::string& value ) { return stoulCast<std::int8_t>( value ); } },
        { typeid(std::int16_t),
            []( const std::string& value ) { return stoulCast<std::int16_t>( value ); } },
        { typeid(std::int32_t),
            []( const std::string& value ) { return stoulCast<std::int32_t>( value ); } },
        { typeid(std::int64_t),
            []( const std::string& value ) { return stoulCast<std::int64_t>( value ); } },
        { typeid(std::uint8_t),
            []( const std::string& value ) { return stoulCast<std::uint8_t>( value ); } },
        { typeid(std::uint16_t),
            []( const std::string& value ) { return stoulCast<std::uint16_t>( value ); } },
        { typeid(std::uint32_t),
            []( const std::string& value ) { return stoulCast<std::uint32_t>( value ); } },
        { typeid(std::uint64_t),
            []( const std::string& value ) { return stoulCast<std::uint64_t>( value ); } },
        { typeid(float),
            []( const std::string& value ) { return static_cast<float>( stof( value ) ); } },
        { typeid(double),
            []( const std::string& value ) { return static_cast<double>( stod( value ) ); } },
    };

    auto parse = factory.find( type );
    if ( parse == factory.end() )
    {
        throw polysync::error( "no string converter defined for terminal type" )
            << exception::type( descriptor::terminalTypeMap.at(type).name )
            << exception::module( "detector" )
            << status::description_error;
    }

    return parse->second(value);
}

// Load the global type detector dictionary detector::catalog with an entry
// from a TOML table.
void load( const std::string& name, std::shared_ptr<cpptoml::table> table,
           Catalog& catalog )
{
    logger log("detector");

    // Recurse nested tables
    if (!table->contains("description"))
    {
        for ( const auto& tp: *table )
            load( name + "." + tp.first, tp.second->as_table(), catalog );
        return;
    }

    if ( !table->contains("detector") )
    {
        BOOST_LOG_SEV(log, severity::debug2)
            << "no sequel types following \"" << name << "\"";
        return;
    }

    if ( !descriptor::catalog.count(name) )
    {
        throw polysync::error( "no type description" )
            << exception::type( name )
            << exception::module( "detector" )
            << status::description_error;
    }

    auto det = table->get( "detector" );

    if ( !det->is_table_array() )
    {
        throw polysync::error( "detector list must be an array" )
            << exception::type( name )
            << exception::module( "detector" )
            << status::description_error;
    }

    for ( const auto& branch: *det->as_table_array() )
    {
        if ( !branch->is_table() )
        {
            throw polysync::error("detector must be a table")
                << exception::type(name)
                << exception::module("detector")
                << status::description_error;
        }

        auto table = branch->as_table();
        if ( !table->contains("name") )
        {
            throw polysync::error( "detector requires a \"name\" field" )
                << exception::type( name )
                << exception::module( "detector" )
                << status::description_error;
        }

        auto sequel = table->get_as<std::string>("name");
        if ( !sequel )
        {
            throw polysync::error( "detector name must be a string" )
                << exception::type( name )
                << exception::module( "detector" )
                << status::description_error;
        }

        decltype( std::declval<detector::Type>().match ) match;
        const descriptor::Type& desc = descriptor::catalog.at(name);
        for ( auto pair: *table )
        {
            if (pair.first == "name") // special field
            {
                continue;
            }

            // Dig through the type description to get the type of the matching field
            auto it = std::find_if( desc.begin(), desc.end(), [pair](auto f)
                    {
                        return f.name == pair.first;
                    });

            // The field name did not match at all; get out of here.
            if ( it == desc.end() )
            {
                throw polysync::error( "unknown field" )
                    << exception::type( name )
                    << exception::detector( pair.first )
                    << exception::field( it->name )
                    << exception::module( "detector" )
                    << status::description_error;
            }

            // Disallow branching on any non-native field type.  Branching on
            // arrays or nested types is not supported (and hopefully never
            // will need to be).
            const std::type_index* idx = it->type.target<std::type_index>();
            if ( !idx )
            {
                throw polysync::error( "illegal branch on compound type" )
                    << exception::type( name )
                    << exception::detector( pair.first )
                    << exception::field( it->name )
                    << exception::module( "detector" )
                    << status::description_error;
            }

            // For this purpose, TOML numbers must be strings because TOML is
            // not very type flexible (and does not know about hex notation).
            // Here is where we convert that string into a strong type.
            std::string value = pair.second->as<std::string>()->get();
            match.emplace( pair.first, parseTerminalFromString( value, *idx ) );
        }


        auto it = std::find_if( detector::catalog.begin(), detector::catalog.end(),
                    [name, sequel](auto f)
                    {
                        return f.parent == name && f.child == *sequel;
                    });

        if ( it != detector::catalog.end() )
        {
            BOOST_LOG_SEV(log, severity::debug2) << "duplicate sequel \""
                << name << "\" -> \"" << *sequel << "\" found but not installed";
            return;
        }

        detector::catalog.emplace_back( detector::Type { name, match, *sequel } );

        BOOST_LOG_SEV(log, severity::debug2) <<  "installed sequel \""
            << detector::catalog.back().parent << "\" -> \""
            << detector::catalog.back().child << "\"";
    }
}

} // namespace detector

std::string detect(const node& parent)
{
    logging::logger log { "detector" };

    polysync::tree tree = *parent.target<polysync::tree>();
    if (tree->empty())
    {
        throw polysync::error("parent tree is empty")
            << exception::module("detector");
    }

    // Iterate each detector in the catalog and check for a match.  Store the
    // resulting type name in tpname.
    std::string tpname;
    for ( const detector::Type& det: detector::catalog )
    {
        // Parent is not even the right type, so short circuit and fail this test early.
        if (det.parent != parent.name)
        {
            BOOST_LOG_SEV(log, severity::debug2) << det.child << " not matched: parent \""
                << parent.name << "\" != \"" << det.parent << "\"";
            continue;
        }

        // Iterate each field in the detector looking for mismatches.
        std::vector<std::string> mismatch;
        for (auto field: det.match)
        {
            auto it = std::find_if(tree->begin(), tree->end(),
                    [field](const node& n) {
                    return n.name == field.first;
                    });
            if (it == tree->end())
            {
                BOOST_LOG_SEV(log, severity::debug2)
                    << det.child << " not matched: parent \""
                    << det.parent << "\" missing field \"" << field.first << "\"";
                break;
            }
            if (*it != field.second)
            {
                mismatch.emplace_back(field.first);
            }
        }

        // Too many matches. Catalog is not orthogonal and needs tweaking.
        if (mismatch.empty() && !tpname.empty())
        {
            throw polysync::error("non-unique detectors: " + tpname + " and " + det.child)
                << exception::module("detector");
        }

        // Exactly one match. We have detected the sequel type.f
        if (mismatch.empty())
        {
            tpname = det.child;
            continue;
        }

        // The detector failed, print a fancy message to help developer fix
        // catalog.  Define the formatting function here statically so the following
        // boost::log macro can manage it's invocation for performance reasons.
        static auto details = [&]( const std::string& field ) -> std::string
        {
            std::stringstream os;
            os << field + ": ";
            auto it = std::find_if(tree->begin(), tree->end(),
                    [field](auto& f){ return field == f.name; });
            os << *it;
            if (det.match.count(field))
            {
                os << (*it == det.match.at(field) ? " == " : " != ");
                eggs::variants::apply([&os](auto& v) { os << v; }, det.match.at(field));
            } else
                os << " missing from description";
            return os.str();
        };

        BOOST_LOG_SEV(log, severity::debug2) << det.child << ": mismatched { "
            << std::accumulate(mismatch.begin(), mismatch.end(), std::string(),
                    [&](const std::string& str, auto& field) { return str + details(field); })
            << " }";
    }

    if (tpname.empty())
    {
        BOOST_LOG_SEV(log, severity::debug1) << "type not detected, returning raw sequence";
        return "raw";
    }

    BOOST_LOG_SEV(log, severity::debug1) << tpname << " matched from parent \"" << parent.name << "\"";

    return tpname;
}


} // namespace polysync::plog


