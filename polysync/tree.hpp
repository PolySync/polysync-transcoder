#pragma once

#include <polysync/plog/core.hpp>
#include <eggs/variant.hpp>
#include <iostream>

namespace polysync { namespace plog {

namespace hana = boost::hana;

// Ugh, eggs::variant lacks the recursive feature supported by the old
// boost::variant, which makes it a PITA to implement a syntax tree (no nested
// variants!).  The std::shared_ptr<tree> is a workaround for this eggs
// limitation.  Apparently we are getting std::variant in C++17; maybe we can
// factor out std::shared_ptr<tree> to just tree, then.

struct node;

struct tree : std::shared_ptr<std::vector<node>> {
    using std::shared_ptr<element_type>::shared_ptr;

    static tree create() { return std::make_shared<element_type>(); }
    static tree create(std::initializer_list<node> init) {
        return std::make_shared<element_type>(init);
    }
};

using bytes = plog::sequence<std::uint32_t, std::uint8_t>;

// Each leaf node in the tree may contain any of a limited set of types, defined here.
using variant = eggs::variant<
    // Nested types and vectors of nested types
    tree, std::vector<tree>,

    // Undecoded raw bytes (fallback when description is missing)
    bytes, 

    // Floating point types and native vectors
    float, std::vector<float>, 
    double, std::vector<double>, 

    // Integer types and native vectors
    std::int8_t, std::vector<std::int8_t>, 
    std::int16_t, std::vector<std::int16_t>, 
    std::int32_t, std::vector<std::int32_t>, 
    std::int64_t, std::vector<std::int64_t>,
    std::uint8_t, std::vector<std::uint8_t>, 
    std::uint16_t, std::vector<std::uint16_t>, 
    std::uint32_t, std::vector<std::uint32_t>, 
    std::uint64_t, std::vector<std::uint64_t>
    >;

// Dynamic parsing builds a tree of nodes to represent the record.  Each leaf
// is strongly typed as one of the variant component types.  A node is just a
// variant with a name.
struct node : variant {

    template <typename T>
    node(const std::string& n, const T& value) : variant(value), name(n) { }

    using variant::variant;
    const std::string name;

    // Convert a hana structure into a vector of dynamic nodes.
    template <typename Struct>
    static node from(const Struct& s, const std::string&);
};

// inline std::ostream& operator<<(std::ostream& os, const node& value) {
//     eggs::variants::apply([&os](auto a) { os << plog::to_string(a); }, value);
//     return os;
// }

// extern std::ostream& operator<<(std::ostream& os, const node& rec);

inline bool operator==(const tree& lhs, const tree& rhs) { 
    if (lhs->size() != rhs->size())
        return false;

    return std::equal(lhs->begin(), lhs->end(), rhs->begin(), rhs->end(), 
            [](const node& ln, const node& rn) {

                // If the two nodes are both trees, recurse.
                const tree* ltree = ln.target<tree>();
                const tree* rtree = rn.target<tree>();
                if (ltree && rtree)
                    return operator==(*ltree, *rtree);

                // Otherwise, just use variant operator==()
                return ln == rn;
            });
}
inline bool operator!=(const tree& lhs, const tree& rhs) { return !operator==(lhs, rhs); }

// Convert a hana structure into a vector of dynamic nodes.
template <typename Struct>
inline node node::from(const Struct& s, const std::string& type) {
    tree tr = tree::create();
    hana::for_each(s, [tr](auto pair) { 
            std::string name = hana::to<char const*>(hana::first(pair));
            tr->emplace_back(name, hana::second(pair));
            });
        
    return node(type, tr);
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
// inline bool operator==(const polysync::plog::variant& lhs, const polysync::plog::variant& rhs) {
//     std::stringstream lrep;
//     std::stringstream rrep;
// 
//     lrep << lhs;
//     rrep << rhs;
// 
//     return lrep.str() == rrep.str();
// }

template <typename U>
std::enable_if_t<std::is_integral<U>::value, bool>
operator==(const polysync::plog::variant& lhs, U const& rhs) 
{  
    // namespace endian = boost::endian;

    // using bigendian = endian::endian_arithmetic<endian::order::big, U, 8*sizeof(U)>;
    // if (lhs.target_type() == typeid(bigendian)) 
    //     return lhs.target<bigendian>()->value() == rhs;

    if (lhs.target_type() == typeid(U))
        return *lhs.target<U>() == rhs;

    return false;
}

template <typename T, typename U>
bool operator!=(const T& lhs, const U& rhs) { return !operator==(lhs, rhs); }

}}


