#include <polysync/transcode/description.hpp>
#include <polysync/transcode/logging.hpp>
#include <polysync/transcode/io.hpp>
#include <polysync/3rdparty/cpptoml.h>

#include <regex>

namespace polysync { namespace plog {

using polysync::logging::logger;
using polysync::logging::severity;

namespace detector { 

std::vector<type> catalog; 


}

namespace descriptor { 
    
std::map<std::string, type> catalog; 

// Load the global type description catalog with an entry from a TOML table.
void load(const std::string& name, std::shared_ptr<cpptoml::table> table) {
    logger log("description[" + name + "]");

    // Recurse nested tables
    if (!table->contains("description")) {
        for (const auto& type: *table) 
            load(name + "." + type.first, type.second->as_table());
        return;
    }

    descriptor::type desc;
    auto dt = table->get("description");
    if (!dt->is_table_array())
        throw std::runtime_error("[description] must be a table array");

    for (std::shared_ptr<cpptoml::table> fp: *dt->as_table_array()) {

        // skip reserved bytes
        if (fp->contains("skip")) {
            int skip = *fp->get_as<int>("skip");
            desc.emplace_back(field { "skip", std::to_string(skip) });
            continue;
        }

        // parse a normal binary field
        if (!fp->contains("name"))
            throw std::runtime_error("[description] lacks manditory \"name\" field");
        if (!fp->contains("type"))
            throw std::runtime_error("[description] lacks \"type\" field");

        desc.emplace_back(field { 
                *fp->get_as<std::string>("name"),
                *fp->get_as<std::string>("type")
                }); 
    }

    catalog.emplace(name, desc);
    BOOST_LOG_SEV(log, severity::debug1) << "installed type description";
    BOOST_LOG_SEV(log, severity::debug2) << name << " = " << desc;

};

} // namespace descriptor

namespace detector {

template <typename T>
inline T hex_stoul(const std::string& value) {
    std::regex is_hex("0x.+");
    if (std::regex_match(value, is_hex))
        return static_cast<T>(std::stoul(value, 0, 16));
    else
        return static_cast<T>(std::stoul(value));
}

// Description strings have type information, but the type comes out as a
// string (because TOML does not have a very powerful type system).  Define
// factory functions to look up a type by string name and convert to a strong type.
static variant convert(const std::string value, const std::string type) {
    static const std::map<std::string, std::function<variant (const std::string&)>> factory = {
        { "uint8", [](const std::string& value) { return hex_stoul<std::uint8_t>(value); } },
        { "uint16", [](const std::string& value) { return hex_stoul<std::uint16_t>(value); } },
        { "uint32", [](const std::string& value) { return hex_stoul<std::uint32_t>(value); } },
        { "uint64", [](const std::string& value) { return hex_stoul<std::uint64_t>(value); } },
        { ">uint16", [](const std::string& value) { return hex_stoul<std::uint16_t>(value); } },
        { ">uint32", [](const std::string& value) { return hex_stoul<std::uint32_t>(value); } },
        { ">uint64", [](const std::string& value) { return hex_stoul<std::uint64_t>(value); } },
        { "float", [](const std::string& value) { return static_cast<float>(stof(value)); } },
        { "double", [](const std::string& value) { return static_cast<double>(stod(value)); } },
    };

    if (!factory.count(type))
        throw std::runtime_error("no string converter known for \"" + type + "\"");

    return factory.at(type)(value);
}

// Load the global type detector dictionary detector::catalog with an entry from a TOML table.
void load(const std::string& name, std::shared_ptr<cpptoml::table> table) {
    logger log("detector[" + name + "]");

    // Recurse nested tables
    if (!table->contains("description")) {
        for (const auto& tp: *table) 
            load(name + "." + tp.first, tp.second->as_table());
        return;
    }

    if (!table->contains("detector")) {
        BOOST_LOG_SEV(log, severity::debug1) << "no sequel";
        return;
    }

    // type dt;
    auto det = table->get("detector");
    if (!det->is_table())
        throw std::runtime_error("detector must be a table");

    for (const auto& branch: *det->as_table()) {

        if (!branch.second->is_table())
            throw std::runtime_error("detector pattern must be a table");
        if (!plog::descriptor::catalog.count(name))
            throw std::runtime_error("no type description for " + name);

        decltype(std::declval<detector::type>().match) match;
        const plog::descriptor::type& desc = plog::descriptor::catalog.at(name);
        for (auto pair: *branch.second->as_table()) {

            // Dig through the type description to get the type of the matching field
            auto it = std::find_if(desc.begin(), desc.end(), [pair](auto f) { return f.name == pair.first; });
            if (it == desc.end())
                throw std::runtime_error(name + " has no field \"" + pair.first + "\""); 

            // For this purpose, TOML numbers must be strings because TOML is
            // not very type flexible (and does not know about hex notation).
            // Here is where we convert that string into a strong type.
            std::string value = pair.second->as<std::string>()->get();
            match.emplace(pair.first, convert(value, it->type));
        }
        detector::catalog.emplace_back(detector::type { name, match, branch.first });
        BOOST_LOG_SEV(log, severity::debug1) <<  "installed sequel \"" << detector::catalog.back().parent 
            << "\" -> \"" << detector::catalog.back().child << "\"";
    }
}

} // namespace detector

}} // namespace polysync::plog
