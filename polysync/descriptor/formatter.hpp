#pragma once

#include <polysync/descriptor/type.hpp>

namespace polysync { namespace descriptor {

// TOML may have a "format" field that specializes how the field is printed to
// the terminal.  The actual printers are defined here, looked up by the name
// provided in the TOML option.

extern std::map< std::string, decltype( Field::format ) > formatFunction;

}} // namespace polysync::descriptor
