#pragma once

#include <deps/cpptoml.h>

#include <polysync/descriptor/catalog.hpp>

namespace polysync { namespace descriptor {
	
// Traverse a TOML table
extern void load( const std::string&, std::shared_ptr<cpptoml::table>, catalog_type& );

}} // namespace polysync::descriptor
