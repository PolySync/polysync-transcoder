#pragma once

#include <map>
#include <string>

#include <boost/hana.hpp>

#include <cpptoml.h>
#include <polysync/tree.hpp>

namespace polysync { namespace detector {

struct Type
{
    // Detectors look at the current node for branching values.
    std::string currentType;

    // One or more fields must match the detector by name and value.
    std::map< std::string, variant > matchField;

    // Given a successful match, this is the type name of the next node.
    std::string nextType;
};

using Catalog = std::vector<Type>;

// The catalog gets loaded from TOML, unit test fixtures, or hardcoding.
extern Catalog catalog;

// Given a parsed TOML table, load the catalog
extern void loadCatalog( const std::string& name, std::shared_ptr<cpptoml::table> );

// Plow through the detector catalog and search for a match against the current
// node.  Return the matching type by it's name.  Zero or multiple matches will
// throw.
extern std::string search( const node& );

}} // namespace polysync::detector


