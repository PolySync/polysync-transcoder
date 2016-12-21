#pragma once

#include <vector>
#include <algorithm>
#include <iostream>

#include <boost/multiprecision/cpp_int.hpp>

#include <eggs/variant.hpp>

#include <polysync/exception.hpp>

// Dynamic parse trees are basically just vectors of "nodes".  A node knows
// it's name and a strongly typed value.

namespace polysync {

// Ugh, eggs::variant lacks the recursive feature supported by the old
// boost::variant, which makes it a PITA to implement a parse tree (no nested
// variants!).  The problem is that we cannot define std::vector<node> before
// we have declared node (as node is an incomplete type unsupported by
// std::vector).  The std::shared_ptr<tree> is a workaround for this eggs
// limitation.  Apparently we are getting std::variant in C++17; maybe we can
// factor out std::shared_ptr<tree> to just tree, then.

struct node;

struct tree : std::shared_ptr< std::vector<node> > {

    tree( const std::string& type ) :
        std::shared_ptr< std::vector<node> >( new std::vector<node>() ),
        type(type) {}

    // This initialization_list<> constructor should only be invoked within
    // unit tests to describe test vectors.  The initialization ends up copy
    // constructing each node which is too slow for performant applications.
    tree( const std::string& type, std::initializer_list<node> init )
        : std::shared_ptr< std::vector<node> >( new std::vector<node>( init ) ),
          type(type) {}

    const std::string type;
};

// Add some context to exceptions
namespace exception {
    using tree = boost::error_info< struct tree_type, tree >;
}

// Fallback to a generic memory chunk when a description is unavailable.
using bytes = std::vector<std::uint8_t>;

// Each node in the tree may contain any of a limited set of strong types.
using variant = eggs::variant<

    // Nested types and vectors of nested types
    tree, std::vector<tree>,

    // Undecoded raw bytes
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
    std::uint64_t, std::vector<std::uint64_t>,

    // Integers longer than 64 bits
    boost::multiprecision::cpp_int, std::vector<boost::multiprecision::cpp_int>
    >;

// Dynamic parsing builds a tree of nodes to represent the record.  Each leaf
// is strongly typed as one of the variant component types.  A node is just a
// variant with a name.
struct node : variant {

    template <typename T>
    node( const std::string& n, const T& value ) : variant( value ), name( n ) { }

    // Factory functions and decoders should return nodes by move for speed
    node( node&& ) = default;

    // The copy constructor should be used sparingly, because the move
    // constructor is faster. Ideally, this constructor is invoked only in
    // unit test vector construction.
    node( const node& ) = default;

    const std::string name;

    // nodes may have a specialized function for formatting the value to a string.
    std::function<std::string ( const variant& )> format;

};

inline bool operator==( const tree& left, const tree& right ) {

    if ( left->size() != right->size() or left.type != right.type )
        return false;

    return std::equal( left->begin(), left->end(), right->begin(), right->end(),
            []( const node& ln, const node& rn ) {

	   	        if ( ln.name != rn.name ) {
		            return false;
                }

                // If the two nodes are both trees, recurse.
                const tree* ltree = ln.target<tree>();
                const tree* rtree = rn.target<tree>();
                if (ltree && rtree) {
                    return operator==(*ltree, *rtree);
	            }

                // Otherwise, just use variant operator==()
                return ln == rn;
            });
}

inline bool operator!=( const tree& lhs, const tree& rhs ) { return !operator==( lhs, rhs ); }

} // namespace polysync

