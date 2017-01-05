#include <typeindex>

#include <polysync/descriptor.hpp>
#include <polysync/detector.hpp>
#include <polysync/exception.hpp>
#include <polysync/logging.hpp>

namespace polysync {

using polysync::logging::logger;
using polysync::logging::severity;

namespace detector {

Catalog catalog;

static logger log( "detector" );

// Helper function to coax an integer out of a hex string.  This would not be
// necessary if TOML supported hex formatted integers natively.
template <typename T>
inline T stoulCast( const std::string& value )
{
    return boost::numeric_cast<T>( std::stoul( value, 0, 0 ) );
}

// Type descriptions have strong type information, but TOML does not have a
// similarly powerful type system, nor does it support integer hex notation.
// parseTerminalFromString coaxes out a strongly typed number from a non-typed
// string (which TOML does support) using the std::type_index from the type
// description.  It supports all integer types and floats, including hex and
// octal notation thanks to std::stoul().
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
            << exception::type( descriptor::terminalTypeMap.at(type).name );
    }

    return parse->second(value);
}

// Plow through the TOML list of detector descriptions and construct a
// descriptor::Type for each one.  Return a catalog of all the detectors found
// in this particular TOML table.
Catalog buildDetectors( const std::string& typeName, std::shared_ptr<cpptoml::table_array> detectorList )
{
    Catalog result;

    for ( const auto& detectorTable: *detectorList )
    {
        if ( !detectorTable->is_table() )
        {
            throw polysync::error( "detector must be a table" );
        }

        auto table = detectorTable->as_table();
        if ( !table->contains( "name" ) )
        {
            throw polysync::error( "detector requires a \"name\" field" );
        }

        auto sequel = table->get_as<std::string>( "name" );
        if ( !sequel )
        {
            throw polysync::error( "detector name must be a string" );
        }

        decltype( std::declval<detector::Type>().matchField ) match;
        const descriptor::Type& description = descriptor::catalog.at( typeName );
        for ( auto pair: *table )
        {
            if (pair.first == "name") // special field is not a match field
            {
                continue;
            }

            // Dig through the type description to get the type of the matching field
            auto it = std::find_if( description.begin(), description.end(),
                    [ pair ]( auto field ) { return field.name == pair.first; });

            // The field name did not match at all; get out of here.
            if ( it == description.end() )
            {
                throw polysync::error( "unknown field" )
                    << exception::detector( pair.first )
                    << exception::field( it->name );
            }

            // Disallow branching on any non-native field type.  Branching on
            // arrays or nested types is not supported (and hopefully never
            // will need to be).
            const std::type_index* idx = it->type.target<std::type_index>();
            if ( !idx )
            {
                throw polysync::error( "illegal branch on compound type" )
                    << exception::detector( pair.first )
                    << exception::field( it->name );
            }

            // For this purpose, TOML numbers must be strings because TOML is
            // not very type flexible (and does not know about hex notation).
            // Here is where we convert that string into a strong type.
            std::string value = pair.second->as<std::string>()->get();
            match.emplace( pair.first, parseTerminalFromString( value, *idx ) );
        }
        result.emplace_back( Type { typeName, match, *sequel } );
    }
    return result;

}

// Load the global type detector dictionary detector::catalog with an entry
// from a TOML table.
void loadCatalog( const std::string& typeName, std::shared_ptr<cpptoml::table> table )
{
    if ( !table->contains( "description" ) )
    {
        // Tables lacking a description field probably have nested tables that
        // do, so recurse into each nested table.
        for ( const auto& nest: *table )
        {
            loadCatalog( typeName + "." + nest.first, nest.second->as_table() );
        }
        return;
    }

    if ( !table->contains( "detector" ) )
    {
        // Missing a detector list is no big deal; it just means that the type is
        // final and always the last part of a message because no sequel will
        // ever follow.
        BOOST_LOG_SEV( log, severity::debug2 )
            << "no sequel types following \"" << typeName << "\"";
        return;
    }

    auto detectorList = table->get( "detector" );

    try
    {
        if ( !descriptor::catalog.count( typeName ) )
        {
            throw polysync::error( "requested detector lacks corresponding type description" );
        }

        if ( !detectorList->is_table_array() )
        {
            throw polysync::error( "detector list must be an array" );
        }

        for ( Type& detect: buildDetectors( typeName, detectorList->as_table_array() ) )
        {
            auto it = std::find_if( catalog.begin(), catalog.end(),
                    [ typeName, detect ]( auto entry ) {
                        return entry.currentType == detect.nextType and
                               entry.nextType == detect.nextType;
                    });

            if ( it != catalog.end() )
            {
                BOOST_LOG_SEV( log, severity::debug2 )
                    << "duplicate sequel \""
                    << typeName << "\" -> \""
                    << detect.nextType
                    << "\" found but not installed";
                continue;
            }

            catalog.emplace_back( detect );

            BOOST_LOG_SEV ( log, severity::debug2 )
                <<  "installed sequel \""
                << detect.currentType << "\" -> \""
                << detect.nextType << "\"";
        }
    }
    catch ( polysync::error& e )
    {
        e << exception::type( typeName );
        e << exception::module( "detector" );
        e << status::description_error;
        throw e;
    }

}

}} // namespace polysync::detector

