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
    try
    {
        return boost::numeric_cast<T>( std::stoul( value, 0, 0 ) );
    }
    catch ( boost::numeric::bad_numeric_cast )
    {
        throw polysync::error( "value overflow on \"" + value + "\"" );
    }
    catch ( std::invalid_argument )
    {
        throw polysync::error( "invalid integer value \"" + value + "\"" );
    }
}

// Type descriptions have strong type information, but TOML does not have a
// similarly powerful type system, nor does it support integer hex notation.
// parseTerminalFromString coaxes out a strongly typed number from a non-typed
// string (which TOML does support) using the std::type_index from the type
// description.  It supports all integer types and floats, including hex and
// octal notation thanks to std::stoul().
Variant parseTerminalFromString( const std::string& value, const std::type_index& type )
{
    static const std::map< std::type_index,
                           std::function< Variant (const std::string&) > > factory =
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
        polysync::error e( "no string converter defined for terminal type" );
        auto it = descriptor::terminalTypeMap.find( type );
        if ( it != descriptor::terminalTypeMap.end() )
        {
            e << exception::type(it->second.name);
        }
        else
        {
            e << exception::type(type.name());
        }
        throw e;
    }

    return parse->second(value);
}

template <typename T>
T checkedTomlGet( std::shared_ptr<cpptoml::table> table, const std::string& key )
{
    if ( !table->contains( key ) )
    {
        throw polysync::error( "detector requires a \"" + key + "\" field" );
    }

    auto result = table->get_as<T>( key );

    if ( !result )
    {
        throw polysync::error( "\"" + key + "\" invalid type" );
    }

    return *result;
}

// Plow through the TOML list of detector descriptions and construct a
// detector::Type for each one.  Return a catalog of all the detectors found
// in this particular TOML table.
Catalog buildDetectors(
        const std::string& nextType,
        std::shared_ptr<cpptoml::table_array> detectorList )
{
    Catalog catalog;

    for ( const auto& tomlTable: *detectorList )
    {
        std::string precursorName = checkedTomlGet<std::string>( tomlTable, "name" );

        // Accumulate a set of (string, Variant) pairs in "match" that must all match the
        // precursor type's payload, in order for the detection to succeed.
        std::map< std::string, Variant> match;

        const descriptor::Type& description = descriptor::catalog.at( precursorName );
        for ( auto pair: *tomlTable )
        {
            std::string key = pair.first;
            std::shared_ptr<cpptoml::base> value = pair.second;

            try
            {
                if ( key == "name") // special field is not a match field
                {
                    continue;
                }

                // Dig through the type description to get the type of the matching field
                auto it = std::find_if( description.begin(), description.end(),
                        [ key ]( auto field ) { return field.name == key; });

                // The field name did not match at all; get out of here.
                if ( it == description.end() )
                {
                    throw polysync::error( "type description lacks detector field" );
                }

                // Disallow branching on any non-native field type.  Branching on
                // arrays or nested types is not supported (and hopefully never
                // will need to be).
                const std::type_index* idx = it->type.target<std::type_index>();
                if ( !idx )
                {
                    throw polysync::error( "illegal key on compound type" );
                }

                // For this purpose, TOML numbers might be strings because TOML is
                // not very type flexible (and does not know about hex notation).
                // Here is where we convert that string into a strong type.
                auto valueString = value->as<std::string>();
                if( valueString )
                {
                    match.emplace( key, parseTerminalFromString( valueString->get(), *idx ) );
                }
                else
                {
                    throw polysync::error( "detector value must be represented as a string" );
                }
            }
            catch ( polysync::error& e )
            {
                e << exception::detector( nextType );
                e << exception::field( key );
                throw e;
            }
        }
        catalog.emplace_back( Type { precursorName, match, nextType } );
    }
    return catalog;

}

// Load the global type detector dictionary detector::catalog with an entry
// from a TOML table.
bool loadCatalog( const std::string& typeName, std::shared_ptr<cpptoml::base> base )
{
    if ( !base->is_table() )
    {
        return false;
    }

    std::shared_ptr<cpptoml::table> table = base->as_table();

    if ( !table->contains( "description" ) )
    {
        // Tables lacking a description field probably have nested tables that
        // do, so recurse into each nested table.
        for ( const auto& nest: *table )
        {
            if ( nest.second->is_table() )
            {
                loadCatalog( typeName + "." + nest.first, nest.second->as_table() );
            }
        }
    }

    if ( !table->contains( "detector" ) and table->contains( "description" ) )
    {
        // Missing a detector list is no big deal; it just means that the type is
        // final and always the last part of a message because no sequel will
        // ever follow.
        BOOST_LOG_SEV( log, severity::debug2 )
            << "no sequel types following \"" << typeName << "\"";
        return false;
    }

    if ( !table->contains( "detector" ) )
    {
        return false;
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
            // Ensure that the detector is unique; there can be no ambiguity
            // about which type comes next.
            for ( auto entry: catalog )
            {
                if ( entry.currentType == detect.currentType and
                        entry.nextType == detect.nextType )
                {
                    throw polysync::error( "ambiguous detector" )
                        << exception::detector( typeName );
                }
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

    return true;
}

}} // namespace polysync::detector

