#include <polysync/description.hpp>
#include <polysync/detector.hpp>
#include <polysync/exception.hpp>
#include <polysync/print_hana.hpp>
#include <polysync/logging.hpp>
#include <polysync/3rdparty/cpptoml.h>

#include <regex>

namespace polysync { 

using polysync::logging::logger;
using polysync::logging::severity;

namespace descriptor { 
    
catalog_type catalog; 

// Load the global type description catalog with an entry from a TOML table.
void load(const std::string& name, std::shared_ptr<cpptoml::table> table, catalog_type& catalog) {
    logger log("description");

    BOOST_LOG_SEV(log, severity::debug1) << "loading \"" << name << "\"";
    try {

        // Recurse nested tables
        if (!table->contains("description")) {
            for (const auto& type: *table) 
                load(name + "." + type.first, type.second->as_table(), catalog);
            return;
        }

        descriptor::type desc(name);
        auto dt = table->get("description");
        if (!dt->is_table_array())
            throw polysync::error("[description] must be a table array");

        for (std::shared_ptr<cpptoml::table> fp: *dt->as_table_array()) {

            // skip reserved bytes
            if (fp->contains("skip")) {
                int size = *fp->get_as<int>("skip");
                desc.emplace_back(field { "skip", skip { size } });
                continue;
            }

            // parse a normal binary field
            if (!fp->contains("name"))
                throw polysync::error("missing required \"name\" field");
            if (!fp->contains("type"))
                throw polysync::error("missing required \"type\" field");

            std::string fname = *fp->get_as<std::string>("name");
            std::string type = *fp->get_as<std::string>("type");

            // Compute what the field.type variant should be
            if (fp->contains("count")) {
                descriptor::array array;

                auto fixed = fp->get_as<size_t>("count");
                auto dynamic = fp->get_as<std::string>("count");
                if (fixed)
                    array.size = *fixed;
                else
                    array.size = *dynamic;
                
                if (descriptor::namemap.count(type))
                    array.type = descriptor::namemap.at(type); // terminal type
                else
                    array.type = type; // nested type

                desc.emplace_back(field { fname, array });
            } else {
                if (descriptor::namemap.count(type))
                    desc.emplace_back(field { fname, descriptor::namemap.at(type) }); 
                else
                    desc.emplace_back(field { fname, descriptor::nested { type } }); 
            }

            // Tune the field description by any optional info
            if (fp->contains("endian"))
                desc.back().bigendian = true;

            if (fp->contains("format")) {
                std::string format = *fp->get_as<std::string>("format");
                if (format == "hex")
                    desc.back().format = [](const node& n) {
                        std::stringstream os;
                        os << std::hex << "0x" << n << std::dec;
                        return os.str();
                    };
                else 
                    BOOST_LOG_SEV(log, severity::warn) << "unknown formatter \"" + format + "\"";
            }
        }

        catalog.emplace(name, desc);
        BOOST_LOG_SEV(log, severity::debug2) << name << " = " << desc;

    } catch (polysync::error& e) {
        // Do not overwrite existing context, as this function is recursive.
        if (!boost::get_error_info<exception::type>(e))
            e << exception::type(name);
        e << exception::module("description");
        throw;
    }
};

lex::lex(const field::variant& v) : std::string(eggs::variants::apply(*this, v)) {}

std::string lex::operator()(std::type_index idx) const {
    return typemap.at(idx).name;
}

std::string lex::operator()(nested idx) const {
    return idx.name;
}

std::string lex::operator()(skip idx) const {
    return "skip(" + std::to_string(idx.size) + ")";
}

std::string lex::operator()(array idx) const {
    return "array<" + eggs::variants::apply(*this, idx.type) + ">(" 
        + eggs::variants::apply(*this, idx.size) + ")";
}

std::string lex::operator()(std::string s) const { return s; }
std::string lex::operator()(size_t s) const { return std::to_string(s); }

std::ostream& operator<<(std::ostream& os, const field& f) {
    using console::format;

    return os << format.fieldname << f.name << ": " 
              << format.value << lex(f.type) << format.normal;
}

std::ostream& operator<<(std::ostream& os, const type& desc) {
    using console::format;

    os << format.tpname << desc.name << ": { " << format.normal;
    std::for_each(desc.begin(), desc.end(), [&os](auto f) { os << f << ", "; });
    return os << format.tpname << desc.name << "}" << format.normal;
}


}} // namespace polysync::descriptor
