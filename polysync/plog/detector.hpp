#pragma once

#include <boost/hana.hpp>
#include <polysync/plog/tree.hpp>
#include <polysync/3rdparty/cpptoml.h>

#include <map>
#include <string>

namespace polysync { namespace plog { 

extern std::string detect(const node&);

namespace detector { 

struct type {
    std::string parent;
    std::map<std::string, variant> match;
    std::string child;
};

using catalog_type = std::vector<type>;
extern catalog_type catalog;

extern void load(const std::string& name, std::shared_ptr<cpptoml::table> table, catalog_type&);

}}} // namespace polysync::plog::detector

BOOST_HANA_ADAPT_STRUCT(polysync::plog::detector::type, parent, match, child);

