#include <polysync/plog/description.hpp>
#include <polysync/plog/detector.hpp>
#include <polysync/plog/io.hpp>
#include <polysync/transcode/logging.hpp>
#include <polysync/3rdparty/cpptoml.h>

#include <regex>

namespace polysync { namespace plog {

using polysync::logging::logger;
using polysync::logging::severity;

namespace descriptor { 
    
catalog_type catalog; 

// Load the global type description catalog with an entry from a TOML table.
void load(const std::string& name, std::shared_ptr<cpptoml::table> table, catalog_type& catalog) {
    logger log("description[" + name + "]");

    // Recurse nested tables
    if (!table->contains("description")) {
        for (const auto& type: *table) 
            load(name + "." + type.first, type.second->as_table(), catalog);
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

}} // namespace polysync::plog
