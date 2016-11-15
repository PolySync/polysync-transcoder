#pragma once

#include <boost/hana.hpp>
#include <polysync/tree.hpp>
#include <polysync/3rdparty/cpptoml.h>

#include <map>
#include <string>

namespace polysync { 

// Plow through the detector catalog and search for a match against a parent
// node.  Return the matching type by it's name.  Zero or multiple matches will throw.
extern std::string detect(const node&);

namespace detector { 

struct type {
    // Detectors all look at their parent node for branching values.
    std::string parent;

    // In the parent, one or more fields must match the detector by name and value.
    std::map<std::string, variant> match;

    // Given a successful match, the child is the name of the matched type.
    std::string child;
};

using catalog_type = std::vector<type>;

// The catalog gets loaded from TOML, unit test fixtures, or hardcoding.
extern catalog_type catalog;

// Given a parsed TOML table, load the catalog 
extern void load(const std::string& name, std::shared_ptr<cpptoml::table> table, catalog_type&);

}} // namespace polysync::detector


