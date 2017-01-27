#pragma once

#include <polysync/descriptor.hpp>

namespace polysync { namespace descriptor {

extern std::ostream& operator<<(std::ostream& os, const Field& f);
extern std::ostream& operator<<(std::ostream& os, const Type& desc);
extern std::ostream& operator<<(std::ostream& os, const BitField& f);

}} // namespace polysync::descriptor


