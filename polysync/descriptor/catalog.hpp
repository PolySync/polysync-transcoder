#pragma once

#include <string>
#include <map>
#include <typeindex>

#include <polysync/descriptor/type.hpp>

namespace polysync { namespace descriptor {

extern std::map< std::string, Type > catalog;

// Very old type catalog support. One or both of these may be able to be
// factored out, given some time to investigate.
extern std::map< std::type_index, Terminal > terminalTypeMap;
extern std::map< std::string, std::type_index > terminalNameMap;

}} // namespace polysync::descriptor
