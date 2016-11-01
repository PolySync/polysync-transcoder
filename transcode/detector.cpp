#include <polysync/plog/description.hpp>
#include <polysync/plog/detector.hpp>
#include <polysync/exception.hpp>
#include <polysync/transcode/logging.hpp>
#include <polysync/plog/io.hpp>

#include <regex>

// Instantiate the static console format; this is used inside of mettle to
// print failure messages through operator<<'s defined in io.hpp.
namespace polysync { namespace console { codes format = color(); }}

namespace polysync { namespace plog { 

using polysync::logging::logger;
using polysync::logging::severity;

namespace detector {

catalog_type catalog; 

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
void load(const std::string& name, std::shared_ptr<cpptoml::table> table, catalog_type& catalog) {
    logger log("detector[" + name + "]");

    // Recurse nested tables
    if (!table->contains("description")) {
        for (const auto& tp: *table) 
            load(name + "." + tp.first, tp.second->as_table(), catalog);
        return;
    }

    if (!table->contains("detector")) {
        BOOST_LOG_SEV(log, severity::debug1) << "no sequel";
        return;
    }

    auto det = table->get("detector");
    if (!det->is_table())
        throw std::runtime_error("detector must be a table");

    for (const auto& branch: *det->as_table()) {

        if (!branch.second->is_table())
            throw polysync::error("detector must be a TOML table") << exception::type(branch.first);

        if (!plog::descriptor::catalog.count(name))
            throw polysync::error("no type description") << exception::type(name);

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

}}} // namespace polysync::plog::detector


