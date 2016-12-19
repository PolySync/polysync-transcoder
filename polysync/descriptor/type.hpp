#pragma once

#include <string>
#include <typeindex>
#include <vector>
#include <iostream>

#include <boost/hana.hpp>

#include <eggs/variant.hpp>

#include <polysync/tree.hpp>

namespace polysync { namespace descriptor {

namespace hana = boost::hana;
	
enum struct byteorder { little_endian, big_endian } ;

struct field;

// A type description is an ordered vector of fields. Each field knows it's own
// name, and it's own type, and the serialization order is the maintained by
// the vector.
struct type : std::vector< field > {

    type( const std::string& n ) : name(n) {}

    // Use the initializer list constructor only for unit test vectors; the
    // copy is too slow for production code.
    type( const char * n, std::initializer_list<field> init )
        : name(n), std::vector<field>( init ) {}

    // The name is the type's key in descriptor::catalog.
    const std::string name;

    bool operator==( const type& rhs ) {
        return name == rhs.name and *this == static_cast< const std::vector<field>& >( rhs );
    }

    bool operator!=( const type& rhs ) {
        return name != rhs.name or *this != static_cast< const std::vector<field>& >( rhs );
    }

};

// Describe a native terminal type like floats and integers.  This struct gets
// instantiated when the "type" field of TOML element is the reserved name of a
// common scalar type, such as "uint16".
struct terminal {
    BOOST_HANA_DEFINE_STRUCT( terminal,
    	( std::string, name),
    	( std::streamoff, size)
    );
};

// Describe a nested type embedded as an element of a higher level type.  The
// nested type must be registered separately in descriptor::catalog.  This
// struct gets instantiated when the "type" field of a TOML element is not a
// native type key (like "uint16").
struct nested {
    BOOST_HANA_DEFINE_STRUCT( nested,
    	( std::string, name )
    );
};

// Describe an arbitrary number of bytes to skip, useful for "reserved" space
// in a binary blob. This struct gets instantiated from the TOML "skip" element. 
struct skip {
    BOOST_HANA_DEFINE_STRUCT( skip, 

    	// Remember how much extra space to skip or add back in, in bytes
        ( std::streamoff, size ),

	// There can be multiple skips in a record, which need to be re-encoded
	// back in order to preserve binary equivalence.  Maybe the vendors put
	// something important there, but left it out of the documentation.  Keep a
	// sort key for each skip, so the order can be preserved on re-encoding.
	( std::uint16_t, order )
    );
};

// Describe an array of terminal or embedded types.  Arrays get complicated,
// because the size may either be a fixed constant in the type definition, or
// read from one of the parent node's fields.  If the "count" field in the TOML
// element is an *integer*, then the array will have fixed size.  If it is a
// *string*, then the string should equal one of the prior fields in the record
// type that yields some type of integer.  This descriptor really covers four
// separate cases between fixed or dynamic size, and native or compound type.
struct array {
    BOOST_HANA_DEFINE_STRUCT( array,

        // Fixed size, or field name to look up.
        ( eggs::variant< size_t, std::string >, size ),

	// Native type's typeid(), or the name of a compound type's description.
	( eggs::variant< std::type_index, std::string >, type )
    );
};

struct field {

    BOOST_HANA_DEFINE_STRUCT( field,
    	( std::string, name ),
    	( eggs::variant< std::type_index, nested, skip, array >, type )
    );

    // Optional metadata about a field may include bigendian storage, and a
    // specialized function to pretty print the value.  Using these attributes
    // is optional.
    descriptor::byteorder byteorder;

    std::function< std::string ( const polysync::variant& ) > format;
};

// Now, reap the benefits of wrapping the structures in boost::hana; equality
// operators and printing metaprograms cover every hana type in one shot.

template <typename S, typename = std::enable_if_t< hana::Struct<S>::value > >
auto operator==( const S& left, const S& right ) {
    return boost::hana::equal( left, right ) ;
}

template <typename S, typename = std::enable_if_t< hana::Struct<S>::value > >
auto operator!=( const S& left, const S& right ) {
    return boost::hana::not_equal( left, right ) ;
}
	
}} // namespace polysync::descriptor

