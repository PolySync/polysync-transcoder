#include <polysync/tree.hpp>
#include <polysync/logging.hpp>
#include <polysync/descriptor/toml.hpp>
#include <polysync/descriptor/formatter.hpp>
#include <polysync/descriptor/print.hpp>
#include <polysync/exception.hpp>
#include <polysync/print_tree.hpp>

namespace polysync { namespace descriptor {

using polysync::logging::logger;
using polysync::logging::severity;

using TablePtr = std::shared_ptr<cpptoml::table>;

struct SkipFactory
{
    std::uint16_t skip_index { 0 };

    bool check( TablePtr table ) const
    {
        return table->contains( "skip" );
    }

    Field operator()( TablePtr table )
    {
        // Skips get an incrementing name, to support sorting and
        // re-serializing in the original order.
        skip_index += 1;
        std::string name = "skip-" + std::to_string(skip_index);

        std::streamoff size = *table->get_as<int>( "skip" );

        return Field { name, Skip { size, skip_index } };
    }
};

struct BitsetFactory
{
    bool check( TablePtr table ) const
    {
        return ( table->contains( "type" ) and table->contains( "count" ) and
                "bit" == *table->get_as<std::string>( "type" ) );
    }

    Bitset operator()( TablePtr table ) const
    {
        std::string name = *table->get_as<std::string>( "name" );
        std::uint8_t count = *table->get_as<std::uint8_t>( "count" );
        return Bitset { name, count };
    }
};

struct BitFactory
{
    bool check( TablePtr table ) const
    {
        return table->contains( "type" )
            and "bit" == *table->get_as<std::string>( "type" )
            and !table->contains( "count" );
    }

    Bit operator()( TablePtr table ) const
    {
        std::string name = *table->get_as<std::string>( "name" );
        return Bit { name };
    }
};

struct BitSkipFactory
{
    std::uint16_t skip_index { 0 };

    bool check( TablePtr table ) const
    {
        return table->contains( "bitskip" );
    }

    BitSkip operator()( TablePtr table )
    {
        return BitSkip { *table->get_as<std::uint8_t>( "bitskip" ) };
    }
};

struct NestedTableFactory
{
    bool check( TablePtr table ) const
    {
        // Lacking a "description" field, the element is probably a TOML array
        // (like "ibeo") containing nested tables that are actual types ( like
        // "ibeo.vehicle_state" ).

        return !table->contains( "description" );
    }

    std::vector<Type> operator()( TablePtr table, const std::string& path ) const
    {
        std::vector<Type> descriptions;

        for ( const auto& type: *table )
        {
            std::string subpath = path.empty() ? type.first : path + "." + type.first;
            std::vector<Type> sublist = loadCatalog( subpath, type.second );
            std::move( sublist.begin(), sublist.end(), std::back_inserter( descriptions ));
        }

        return descriptions;
    }
};

struct ArrayFactory
{
    TablePtr table;

    bool check()
    {
        return table->contains("count");
    }

    Field operator()()
    {
        descriptor::Array array;

        if ( table->get_as<size_t>("count") )
        {
            array.size = *table->get_as<size_t>("count");
        }
        else
        {
            array.size = *table->get_as<std::string>("count");
        }

        std::string type = *table->get_as<std::string>("type");
        if ( descriptor::terminalNameMap.count(type) )
        {
            array.type = descriptor::terminalNameMap.at(type); // terminal type
        }
        else
        {
            array.type = type; // nested type
        }

        std::string fname = *table->get_as<std::string>("name");
        return Field { fname, array };
    }
};

struct FieldFactory
{
    TablePtr table;

    Field operator()() const
    {
        Field field = construct( table );
        setEndian( field );
        setFormat( field );
        return field;
    }

private:

    Field construct( TablePtr table ) const
    {
        if ( !table->contains("name") )
        {
            throw polysync::error("missing required \"name\" field");
        }

        if ( !table->contains("type") )
        {
            throw polysync::error("missing required \"type\" field");
        }

        std::string fname = *table->get_as<std::string>("name");
        std::string type = *table->get_as<std::string>("type");

        ArrayFactory array { table };
        if ( array.check() )
        {
            return array();
        }

        if (descriptor::terminalNameMap.count(type))
        {
            return Field { fname, descriptor::terminalNameMap.at(type) };
        }
        else
        {
            return Field { fname, descriptor::Nested { type } };
        }
    }

    void setEndian( Field& field ) const
    {
        field.byteorder = table->contains("endian") ? ByteOrder::BigEndian : ByteOrder::LittleEndian;
    }

    void setFormat( Field& field ) const
    {
        if (table->contains("format"))
        {
            std::string formatSpecial = *table->get_as<std::string>("format");

            if ( !formatFunction.count( formatSpecial ) )
            {
                throw polysync::error( "unsupported formatter \"" + formatSpecial + "\"" );
            }

            field.format = formatFunction.at( formatSpecial );
        }
    }
};

struct BitFieldFactory
{
    BitFactory bit;
    BitsetFactory bitset;
    BitSkipFactory bitSkip;

    bool check( TablePtr table ) const
    {
        return bit.check( table ) or bitset.check( table) or bitSkip.check( table );
    }

    Field operator()( cpptoml::table_array::iterator& current, cpptoml::table_array::iterator end ) const
    {
        BitField bitfield;
        BitFactory bit;
        BitsetFactory bitset;
        BitSkipFactory bitskip;
        while ( current != end and check( *current) )
        {
            if ( bitskip.check( *current ) )
            {
                bitfield.fields.emplace_back( bitskip( *current ) );
            }
            if ( bit.check( *current ) )
            {
                bitfield.fields.emplace_back( bit(*current) );
            }
            if ( bitset.check( *current ) )
            {
                bitfield.fields.emplace_back( bitset(*current) );
            }
            ++current;
        }
        return Field { std::string(), bitfield };
    }

 };



// Decode a TOML table into type descriptors
std::vector<Type> loadCatalog( const std::string& name, std::shared_ptr<cpptoml::base> element )
{
    logger log("descriptor");

    if ( !element->is_table() )
    {
        return std::vector<Type>();
    }

    BOOST_LOG_SEV(log, severity::debug2) << "loading \"" << name << "\"";

    TablePtr table = element->as_table();
    try
    {
        NestedTableFactory nestedTable;
        if ( nestedTable.check( table ) )
        {
            return nestedTable( table, name );
        }

        // Otherwise, we are expecting a well formed TOML type description.
        std::shared_ptr<cpptoml::base> descriptionTable = table->get("description");
        if ( !descriptionTable->is_table_array() )
        {
            throw polysync::error("[description] must be a TOML table array");
        }

        descriptor::Type description(name);

        SkipFactory skip;
        auto tableArray = descriptionTable->as_table_array();
        BitFieldFactory bitfield;
        for ( auto it = tableArray->begin(); it != tableArray->end(); ++it )
        {
            if ( skip.check( *it ) )
            {
                description.emplace_back( skip( *it ) );
                continue;
            }

            if ( bitfield.check( *it ) )
            {
                description.emplace_back( bitfield( it, tableArray->end() ) );
                // it gets incremented as a side effect of bitfield()
                if ( it == tableArray->end() )
                {
                    break;
                }
                continue;
            }

            FieldFactory field { *it };
            description.emplace_back( field() );
        }

        return std::vector<Type> { description };

    }
    catch ( polysync::error& e )
    {
        // Do not overwrite existing context, as this function is recursive.
        if ( !boost::get_error_info<exception::type>(e) )
        {
            e << exception::type( name );
            e << exception::module( "description" );
        }
        throw;
    }
};

}} // namespace polysync::descriptor
