#pragma once

#include <string>
#include <map>
#include <typeindex>

#include <polysync/descriptor/type.hpp>

namespace polysync { namespace descriptor {

using catalog_type = std::map< std::string, type >;

// Global type descriptor catalogs
extern catalog_type catalog;
extern std::map< std::type_index, terminal > typemap;
extern std::map< std::string, std::type_index > namemap;

}} // namespace polysync::descriptor
