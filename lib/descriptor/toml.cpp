#include <polysync/tree.hpp>
#include <polysync/logging.hpp>
#include <polysync/descriptor/toml.hpp>
#include <polysync/descriptor/print.hpp>
#include <polysync/exception.hpp>
#include <polysync/print_tree.hpp>

namespace polysync { namespace descriptor {

using polysync::logging::logger;
using polysync::logging::severity;

using TablePtr = std::shared_ptr<cpptoml::table>;

struct SkipFactory
{
    Field operator()( TablePtr table )
    {
        std::streamoff size = *table->get_as<int>("skip");
        skip_index += 1;
        std::string name = "skip-" + std::to_string(skip_index);
        return Field { name, Skip { size, skip_index } };
    }

    std::uint16_t skip_index { 0 };
};

struct NestedTableFactory
{
    std::vector<Type> operator()( TablePtr table, const std::string& path )
    {
        std::vector<Type> descriptions;
        for ( const auto& type: *table )
        {
            std::string subpath = path.empty() ? type.first : path + "." + type.first;
            std::vector<Type> sublist = fromToml( type.second->as_table(), subpath );
            std::move( sublist.begin(), sublist.end(), std::back_inserter( descriptions ));
        }
        return descriptions;
    }
};


// TOML may have a "format" field that specializes how the field is printed to
// the terminal.  The actual printers are defined here, looked up by the name
// provided in the TOML option.
std::map< std::string, decltype( Field::format ) > formatFunction {
    { "hex", [](const variant& n)
        {
            // The hex formatter just uses the default operator<<(), but kicks on std::hex first.
            std::stringstream os;
            eggs::variants::apply([&os](auto v) { os << std::hex << "0x" << v << std::dec; }, n);
            return os.str();
        }
    },
};

struct FieldFactory
{
    Field operator()( TablePtr table )
    {
        Field field = construct( table );
        setEndian( field, table );
        setFormat( field, table );
        return field;
    }

    Field construct( TablePtr table )
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

        // Compute what the field.type variant should be
        if (table->contains("count"))
        {
            descriptor::Array array;

            auto fixed = table->get_as<size_t>("count");
            auto dynamic = table->get_as<std::string>("count");
            if ( fixed )
                array.size = *fixed;
            else
                array.size = *dynamic;

            if ( descriptor::terminalNameMap.count(type) )
                array.type = descriptor::terminalNameMap.at(type); // terminal type
            else
                array.type = type; // nested type

            return Field { fname, array };

        } else {

            if (descriptor::terminalNameMap.count(type))
            {
                return Field { fname, descriptor::terminalNameMap.at(type) };
            }
            else
            {
                return Field { fname, descriptor::Nested { type } };
            }
        }

    }

    void setEndian( Field& field, TablePtr table )
    {
        field.byteorder = table->contains("endian") ? ByteOrder::BigEndian : ByteOrder::LittleEndian;
    }

    void setFormat( Field&  field, TablePtr table )
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

// Decode a TOML table into type descriptors
std::vector<Type> fromToml( TablePtr table, const std::string& name )
{
    logger log("TOML");

    BOOST_LOG_SEV(log, severity::debug2) << "loading \"" << name << "\"";

    FieldFactory factory;
    SkipFactory skip;
    NestedTableFactory recurse;

    try {

        // Lacking a "description" field, the element is probably a TOML array
        // (like "ibeo") containing nested tables that are actual types ( like
        // "ibeo.vehicle_state" ).  Recurse into the nested table, keeping
        // track of the path name.
        if ( !table->contains("description") )
        {
            return recurse( table, name );
        }

        // Otherwise, we are expecting a well formed TOML type description.
        std::shared_ptr<cpptoml::base> descriptionTable = table->get("description");
        if ( !descriptionTable->is_table_array() )
        {
            throw polysync::error("[description] must be a TOML table array");
        }

        descriptor::Type description(name);

        for ( TablePtr tomlElement: *descriptionTable->as_table_array() )
        {
            if (tomlElement->contains("skip")) {
                description.emplace_back( skip ( tomlElement ) );
                continue;
            }

            description.emplace_back( fieldFactory ( tomlElement) );
        }

        BOOST_LOG_SEV(log, severity::debug2) << name << " = " << description;
        return std::vector<Type> { description };

    } catch (polysync::error& e) {

        // Do not overwrite existing context, as this function is recursive.
        if (!boost::get_error_info<exception::type>(e))
        {
            e << exception::type(name);
            e << exception::module("description");
        }
        throw;
    }
};

}} // namespace polysync::descriptor
