#pragma once

#include <polysync/plog/core.hpp>
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
struct tree : std::shared_ptr<std::vector<node>> {
    using std::shared_ptr<element_type>::shared_ptr;

    static tree create() { return std::make_shared<element_type>(); }
    static tree create(std::initializer_list<node> init) {
        return std::make_shared<element_type>(init);
    }
};

using bytes = plog::sequence<std::uint32_t, std::uint8_t>;

using variant = eggs::variant<
    tree, bytes, float, double,
    std::int8_t, std::int16_t, int32_t, int64_t,
    std::uint8_t, std::uint16_t, uint32_t, uint64_t,
    endian::big_uint16_t, endian::big_uint32_t, endian::big_uint64_t,
    endian::big_int16_t, endian::big_int32_t, endian::big_int64_t
    >;

// Dynamic parsing builds a tree of nodes to represent the record.  Each leaf
// is strongly typed as one of the variant component types.
struct node : variant {

    template <typename T>
        node(const T& value, const std::string& n) : variant(value), name(n) { }

    using variant::variant;
    std::string name;

    // Convert a hana structure into a vector of dynamic nodes.
    template <typename Struct>
        static node from(const Struct& s, const std::string&);
};

inline bool operator==(const tree& lhs, const tree& rhs) { 
    if (lhs->size() != rhs->size())
        return false;

    return std::equal(lhs->begin(), lhs->end(), rhs->begin(), rhs->end(), 
            [](const node& ln, const node& rn) {

                // If the two nodes are both trees, recurse.
                const tree* ltree = ln.target<tree>();
                const tree* rtree = ln.target<tree>();
                if (ltree && rtree)
                    return operator==(*ltree, *rtree);

                // Otherwise, just use variant operator==()
                return ln == rn;
            });
}


// Convert a hana structure into a vector of dynamic nodes.
template <typename Struct>
inline node node::from(const Struct& s, const std::string& type) {
    tree tr = tree::create();
    hana::for_each(s, [tr](auto pair) { 
            std::string name = hana::to<char const*>(hana::first(pair));
            tr->emplace_back(hana::second(pair), name);
            });
        
    return node(tr, type);
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


