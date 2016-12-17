#pragma once

#include <deps/cpptoml.h>

namespace polysync { namespace descriptor {
	
// Traverse a TOML table
extern void load( const std::string& name, std::shared_ptr<cpptoml::table> table, catalog_type& );

}} // namespace polysync::descriptor
