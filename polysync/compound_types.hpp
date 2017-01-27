#pragma once

#include <vector>
#include <string>

namespace polysync {

// a sequence<LenType, T> is just a vector<T> that knows to read it's length as a LenType
template <typename LenType, typename T>
struct sequence : std::vector<T>
{
    using length_type = LenType;
};

// specialize the std::uint8_t sequences because they are actually strings
template <typename LenType>
struct sequence<LenType, std::uint8_t> : std::string
{
    using std::string::string;
    using length_type = LenType;
};

} // namespace polysync
