#pragma once

#include <polysync/transcode/core.hpp>
#include <eggs/variant.hpp>
#include <boost/endian/arithmetic.hpp>
#include <iostream>

namespace polysync { namespace plog {

namespace endian = boost::endian;
namespace hana = boost::hana;

// Ugh, eggs::variant lacks the recursive feature supported by the old
// boost::variant, which makes it a PITA to implement a syntax tree (no nested
// variants!).  The std::shared_ptr<tree> is a workaround for this eggs
// limitation.  Apparently we are getting std::variant in C++17; maybe we can
// factor out std::shared_ptr<tree> to just tree, then.

struct node;
using tree = std::shared_ptr<std::vector<node>>;
using variant = eggs::variant<
    tree,
    plog::sequence<std::uint32_t, std::uint8_t>,
    float, double,
    std::int8_t, std::int16_t, int32_t, int64_t,
    std::uint8_t, std::uint16_t, uint32_t, uint64_t,
    endian::big_uint16_t, endian::big_uint32_t, endian::big_uint64_t,
    endian::big_int16_t, endian::big_int32_t, endian::big_int64_t
    >;

// Dynamic parsing builds a tree of nodes to represent the record.  Each leaf
// is strongly typed as one of the variant component types.
struct node : variant {

    template <typename T>
    node(const T& value, const std::string& n, const std::string& tp) : variant(value), name(n), type(tp) { }

    using variant::variant;
    std::string name;
    std::string type;

    // Convert a hana structure into a vector of dynamic nodes.
    template <typename Struct>
    static node from(const Struct& s, const std::string& type);
 };

// Convert a hana structure into a vector of dynamic nodes.
template <typename Struct>
inline node node::from(const Struct& s, const std::string& type) {
    tree tr = std::make_shared<tree::element_type>();
    hana::for_each(s, [tr](auto pair) { 
            std::string name = hana::to<char const*>(hana::first(pair));
            tr->emplace_back(hana::second(pair), name, "fixme");
            });
        
    return node(tr, type, type);
}


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
// values are equal, even when the types are different.  Without this
// specialization, the detectors defined in the TOML files mismatch perfectly
// good values just because numbers like 16 are the wrong integer type or the
// wrong endianess.

// The operator== *must* be defined in the eggs namespace to be found.
namespace eggs { namespace variants {

// For variant on variant equality, just compare the lexical representation.
// This is really much more user friendly, if not technically typesafe.
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

template <typename T, typename U>
bool operator!=(const T& lhs, const U& rhs) { return !operator==(lhs, rhs); }

}}


