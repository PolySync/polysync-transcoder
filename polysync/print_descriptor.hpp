#pragma once

#include <polysync/descriptor.hpp>

namespace polysync { namespace descriptor {

extern std::ostream& operator<<(std::ostream& os, const field& f);
extern std::ostream& operator<<(std::ostream& os, const type& desc);

}} // namespace polysync::descriptor


