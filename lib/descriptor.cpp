#include <regex>

#include <eggs/variant.hpp>

#include <deps/cpptoml.h>

#include <polysync/tree.hpp>
#include <polysync/descriptor.hpp>
#include <polysync/detector.hpp>
#include <polysync/exception.hpp>
#include <polysync/print_tree.hpp>
#include <polysync/print_hana.hpp>
#include <polysync/print_descriptor.hpp>
#include <polysync/logging.hpp>
#include <polysync/descriptor/lex.hpp>

namespace polysync {

using polysync::logging::logger;
using polysync::logging::severity;

namespace descriptor {

TypeCatalog catalog;

// Load the global type description catalog with an entry from a TOML table.
std::vector<Type> fromToml( std::shared_ptr<cpptoml::table> table, const std::string& name )
{
    logger log("description");

    std::vector<Type> result;

    BOOST_LOG_SEV(log, severity::debug2) << "loading \"" << name << "\"";
    try {

        // Recurse nested tables
        if (!table->contains("description"))
        {
            for (const auto& type: *table)
            {
                std::string subpath = name.empty() ? type.first : name + "." + type.first;
                std::vector<Type> sublist = fromToml( type.second->as_table(), subpath );
                std::move( sublist.begin(), sublist.end(), std::back_inserter( result ));
            }
            return result;
        }

        descriptor::Type desc(name);
        auto dt = table->get("description");
        if (!dt->is_table_array())
        {
            throw polysync::error("[description] must be a table array");
        }

        std::uint16_t skip_index = 1;
        for (std::shared_ptr<cpptoml::table> fp: *dt->as_table_array())
        {
            // skip reserved bytes
            if (fp->contains("skip")) {
                std::streamoff size = *fp->get_as<int>("skip");
                std::string name = "skip-" + std::to_string(skip_index);
                desc.emplace_back( Field { name, Skip { size, skip_index } } );
                skip_index += 1;
                continue;
            }

            // parse a normal binary field
            if (!fp->contains("name"))
            {
                throw polysync::error("missing required \"name\" field");
            }

            if (!fp->contains("type"))
            {
                throw polysync::error("missing required \"type\" field");
            }

            std::string fname = *fp->get_as<std::string>("name");
            std::string type = *fp->get_as<std::string>("type");

            // Compute what the field.type variant should be
            if (fp->contains("count"))
            {
                descriptor::Array array;

                auto fixed = fp->get_as<size_t>("count");
                auto dynamic = fp->get_as<std::string>("count");
                if (fixed)
                    array.size = *fixed;
                else
                    array.size = *dynamic;

                if (descriptor::terminalNameMap.count(type))
                    array.type = descriptor::terminalNameMap.at(type); // terminal type
                else
                    array.type = type; // nested type

                desc.emplace_back(Field { fname, array });
            } else {
                if (descriptor::terminalNameMap.count(type))
                    desc.emplace_back(Field { fname, descriptor::terminalNameMap.at(type) });
                else
                    desc.emplace_back(Field { fname, descriptor::Nested { type } });
            }

            // Tune the field description by any optional info
	    desc.back().byteorder = fp->contains("endian") ? ByteOrder::BigEndian : ByteOrder::LittleEndian;

            if (fp->contains("format")) {
                std::string format = *fp->get_as<std::string>("format");
                if (format == "hex")
                    desc.back().format = [](const variant& n) {
                        std::stringstream os;
                        eggs::variants::apply([&os](auto v) { os << std::hex << "0x" << v << std::dec; }, n);
                        return os.str();
                    };
                else
                    BOOST_LOG_SEV(log, severity::warn) << "unknown formatter \"" + format + "\"";
            }
        }

        // catalog.emplace(name, desc);
        BOOST_LOG_SEV(log, severity::debug2) << name << " = " << desc;
        result.push_back(desc);
        return result;

    } catch (polysync::error& e) {
        // Do not overwrite existing context, as this function is recursive.
        if (!boost::get_error_info<exception::type>(e))
            e << exception::type(name);
        e << exception::module("description");
        throw;
    }
};

std::string lex::operator()(std::type_index idx) const {
    return terminalTypeMap.at(idx).name;
}

std::string lex::operator()(Nested idx) const {
    return idx.name;
}

std::string lex::operator()(Skip idx) const {
    return "skip-" + std::to_string(idx.order) + "(" + std::to_string(idx.size) + ")";
}

std::string lex::operator()(Array idx) const {
    return "array<" + eggs::variants::apply(*this, idx.type) + ">("
        + eggs::variants::apply(*this, idx.size) + ")";
}

std::string lex::operator()(std::string s) const { return s; }
std::string lex::operator()(size_t s) const { return std::to_string(s); }

std::ostream& operator<<(std::ostream& os, const Field& f) {
    return os << format->fieldname(f.name + ": ") << format->value(lex(f.type));}

std::ostream& operator<<(std::ostream& os, const Type& desc) {
    os << format->type(desc.name + ": { ");
    std::for_each(desc.begin(), desc.end(), [&os](auto f) { os << f << ", "; });
    os.seekp( -2, std::ios_base::end ); // remove that last comma
    return os << format->type(" }");
}

std::map<std::string, std::type_index> terminalNameMap {
    { "int8", typeid(std::int8_t) },
    { "int16", typeid(std::int16_t) },
    { "int32", typeid(std::int32_t) },
    { "int64", typeid(std::int64_t) },
    { "uint8",  typeid(std::uint8_t) },
    { "uint16", typeid(std::uint16_t) },
    { "uint32", typeid(std::uint32_t) },
    { "uint64", typeid(std::uint64_t) },
    { "float", typeid(float) },
    { "float32", typeid(float) },
    { "double", typeid(double) },
};

std::map<std::type_index, descriptor::Terminal> terminalTypeMap {
    { typeid(std::int8_t), { "int8", sizeof(std::int8_t) } },
    { typeid(std::int16_t), { "int16", sizeof(std::int16_t) } },
    { typeid(std::int32_t), { "int32", sizeof(std::int32_t) } },
    { typeid(std::int64_t), { "int64", sizeof(std::int64_t) } },
    { typeid(std::uint8_t), { "uint8", sizeof(std::uint8_t) } },
    { typeid(std::uint16_t), { "uint16", sizeof(std::uint16_t) } },
    { typeid(std::uint32_t), { "uint32", sizeof(std::uint32_t) } },
    { typeid(std::uint64_t), { "uint64", sizeof(std::uint64_t) } },
    { typeid(float), { "float", sizeof(float) } },
    { typeid(double), { "double", sizeof(double) } },
};

}} // namespace polysync::descriptor
