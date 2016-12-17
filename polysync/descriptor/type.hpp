#pragma once

#include <string>
#include <typeindex>
#include <vector>
#include <iostream>

#include <boost/hana/define_struct.hpp>
#include <boost/hana/comparing.hpp>

#include <polysync/tree.hpp>
#include <polysync/hana.hpp>
#include <polysync/print_tree.hpp>

namespace polysync { namespace descriptor {

// Modeling all of the descriptors using boost::hana allows using generic
// equality operators and generic printers, eliminating a ton of repetition.

template <typename S, typename = std::enable_if_t<hana::Struct<S>::value> >
bool operator==( const S& left, const S& right ) {
    return boost::hana::equal( left, right) ;
}
	
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
// nested type must also be registered in descriptor::catalog.  This struct
// gets instantiated when the "type" field of a TOML element is not a native
// type key (like "uint16").
struct nested {
    BOOST_HANA_DEFINE_STRUCT( nested,
        // Key of the embedded type to search in the descriptor::catalog.
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
        ( eggs::variant<size_t, std::string>, size ),

	// Native type's typeid(), or the name of a compound type's description.
	( eggs::variant<std::type_index, std::string>, type )
    );
};

struct field {
    using variant = eggs::variant<std::type_index, nested, skip, array>;

    BOOST_HANA_DEFINE_STRUCT( field,
    	( std::string, name ),
    	( variant, type )
    );

    // Optional metadata about a field may include bigendian storage, and a
    // specialized function to pretty print the value.  Using these attributes
    // is optional.
    bool bigendian { false };

    std::function<std::string ( const polysync::variant& )> format;
};

inline bool operator==( const field& lhs, const field& rhs ) {
    return lhs.name == rhs.name && lhs.type == rhs.type;
}

// The full type description is just a vector of fields plus a name for the
// compound type.  This has to be a vector, not a map, to preserve the
// serialization order in the plog flat file.
struct type : std::vector<field> {

    type(const std::string& n) : name(n) {}
    type(const char * n, std::initializer_list<field> init)
        : name(n), std::vector<field>(init) {}

    // This is the name of the type.  Each element of the vector also has a
    // name, which identifies the field.
    const std::string name;
};

inline bool operator==( const type& lhs, const type& rhs ) {
    return lhs.name == rhs.name and static_cast<const std::vector<field>&>(lhs) == rhs;
}

inline bool operator!=( const type& lhs, const type& rhs ) {
    return lhs.name != rhs.name or static_cast<const std::vector<field>&>(lhs) != rhs;
}

}} // namespace polysync::descriptor

