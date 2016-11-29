#pragma once

#include <polysync/exception.hpp>

#include <eggs/variant.hpp>
#include <vector>
#include <algorithm>
#include <iostream>
#include <boost/optional.hpp>
#include <boost/hana.hpp>

// Dynamic parse trees are basically just vectors of "nodes".  A node knows
// it's name and a strongly typed value.

namespace polysync { 

namespace hana = boost::hana;

// Ugh, eggs::variant lacks the recursive feature supported by the old
// boost::variant, which makes it a PITA to implement a syntax tree (no nested
// variants!).  The std::shared_ptr<tree> is a workaround for this eggs
// limitation.  Apparently we are getting std::variant in C++17; maybe we can
// factor out std::shared_ptr<tree> to just tree, then.

struct node;

struct tree : std::shared_ptr<std::vector<node>> {
    std::string type;

    static tree create(const std::string& type) { 
        tree res(type);
        // res.type = type;
        return res;
    }

    static tree create(const std::string& type, std::initializer_list<node> init) {
        tree res(type, init);
        // res.type = type;
        return std::move(res);
    }

    tree() : std::shared_ptr<std::vector<node>>(new std::vector<node>()) {}
    tree(const std::string& type) : std::shared_ptr<std::vector<node>>(new std::vector<node>()), type(type) {}
    tree(const std::string& type, std::initializer_list<node> init) : std::shared_ptr<std::vector<node>>(new std::vector<node>(init)), type(type) {}
};

// Add some context to exceptions
namespace exception {
using tree = boost::error_info<struct tree_type, tree>;
}

// Fallback to a generic memory chunk when a description is unavailable.
using bytes = std::vector<std::uint8_t>;

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
    std::uint8_t, 
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
    const std::string name;

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
    tree tr = tree::create(type);
    hana::for_each(s, [tr](auto pair) { 
            std::string name = hana::to<char const*>(hana::first(pair));
            tr->emplace_back(name, hana::second(pair));
            });
        
    return node(type, tr);
}

inline std::ostream& operator<<(std::ostream&, const tree&); 

} // namespace polysync

#include <polysync/print.hpp>

namespace polysync {

inline std::ostream& operator<<(std::ostream& os, const node& n) {
    if (n.target_type() == typeid(std::uint8_t))
            return os << static_cast<std::uint16_t>(*n.target<std::uint8_t>());
    if (n)
        return eggs::variants::apply([&os](auto a) -> std::ostream& { return os << a; }, n);
    return os << "unset";
}


inline std::ostream& operator<<(std::ostream& os, const tree& t) {
    return os << *t;
}

} // polysync


