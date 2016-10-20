#pragma once

#include <polysync/transcode/core.hpp>
#include <eggs/variant.hpp>
#include <boost/endian/arithmetic.hpp>
#include <iostream>

namespace polysync { namespace plog {

namespace endian = boost::endian;

struct tree;

// Ugh, eggs::variant lacks the recursive feature supported by the old
// boost::variant, which makes it a PITA to implement a syntax tree (no nested
// variants!).  The std::shared_ptr<tree> is a workaround for this eggs
// limitation.  Apparently we are getting std::variant in C++17; maybe we can
// factor out std::shared_ptr<tree> to just tree, then.
using variant = eggs::variant<
    std::shared_ptr<tree>,
    msg_header,
    plog::sequence<std::uint32_t, std::uint8_t>,
    float, double,
    std::int8_t, std::int16_t, int32_t, int64_t,
    std::uint8_t, std::uint16_t, uint32_t, uint64_t,
    endian::big_uint16_t, endian::big_uint32_t, endian::big_uint64_t,
    endian::big_int16_t, endian::big_int32_t, endian::big_int64_t
    >;

}} // namespace polysync::plog

inline std::ostream& operator<<(std::ostream& os, const polysync::plog::variant& value) {
    if (value)
        return eggs::variants::apply([&os](auto a) -> std::ostream& { return os << a; }, value);
    return os << "unset";
}

// eggs::variant has a nice default comparison operator==() implemented.
// However, it is very semantically strict that if the types do not exactly
// match, equality fails.  Here, operator==() must be specialized for
// plog::variant do relax this constraint a bit.  In particular, we need
// comparisons between little and big endian numbers to pass when the decoded
// values are equal, even when the types are different.

// The operator== *must* be defined in the eggs namespace to be found.
namespace eggs { namespace variants {

// For variant on variant equality, just compare the lexical representation.
inline bool operator==(const polysync::plog::variant& lhs, const polysync::plog::variant& rhs) {
    std::stringstream lrep;
    std::stringstream rrep;

    lrep << lhs;
    rrep << rhs;

    return lrep.str() == rrep.str();
}

template <typename U>
std::enable_if_t<std::is_integral<U>::value, bool>
operator==(const polysync::plog::variant& lhs, U const& rhs) 
{  
    namespace endian = boost::endian;

    using bigendian = endian::endian_arithmetic<endian::order::big, U, 8*sizeof(U)>;
    if (lhs.target_type() == typeid(bigendian)) 
        return lhs.target<bigendian>()->value() == rhs;

    if (lhs.target_type() == typeid(U))
        return *lhs.target<U>() == rhs;

    return false;
}

}}


