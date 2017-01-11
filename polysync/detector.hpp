#pragma once

#include <map>
#include <string>

#include <boost/hana.hpp>

#include <cpptoml.h>
#include <polysync/tree.hpp>

namespace polysync {

// Plow through the detector catalog and search for a match against a parent
// node.  Return the matching type by it's name.  Zero or multiple matches will throw.
extern std::string detect( const node& );

namespace detector {

struct Type {
    // Detectors all look at their parent node for branching values.
    std::string parent;

    // In the parent, one or more fields must match the detector by name and value.
    std::map< std::string, variant > match;

    // Given a successful match, the child is the name of the matched type.
    std::string child;
};

using Catalog = std::vector<Type>;

// The catalog gets loaded from TOML, unit test fixtures, or hardcoding.
extern Catalog catalog;

// Given a parsed TOML table, load the catalog
extern void load( const std::string& name, std::shared_ptr<cpptoml::table> table, Catalog& );

}} // namespace polysync::detector


