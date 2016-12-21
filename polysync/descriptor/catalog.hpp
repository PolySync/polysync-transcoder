#pragma once

#include <string>
#include <map>
#include <typeindex>

#include <polysync/descriptor/type.hpp>

namespace polysync { namespace descriptor {

using TypeCatalog = std::map< std::string, Type >;

// Global type descriptor catalogs
extern TypeCatalog catalog;
extern std::map< std::type_index, Terminal > terminalTypeMap;
extern std::map< std::string, std::type_index > terminalNameMap;

}} // namespace polysync::descriptor
