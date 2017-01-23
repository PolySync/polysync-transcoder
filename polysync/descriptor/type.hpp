#pragma once

#include <string>
#include <typeindex>
#include <vector>
#include <iostream>
#include <numeric>

#include <boost/hana.hpp>

#include <eggs/variant.hpp>

#include <polysync/tree.hpp>

namespace hana = boost::hana;
namespace polysync { namespace descriptor {

enum struct ByteOrder { LittleEndian, BigEndian } ;

struct Field;

// A type description is an ordered vector of Fields. Each Field knows it's own
// name, and it's own type, and the serialization order is the maintained by
// the vector.
struct Type : std::vector<Field>
{
    Type( const std::string& n ) : name(n) {}

    // Use the initializer list constructor only for unit test vectors; the
    // copy is too slow for production code.
    Type( const char * n, std::initializer_list<Field> init )
        : name(n), std::vector<Field>( init ) {}

    // The name is the type's key in descriptor::catalog.
    const std::string name;

};

// Describe a native terminal type like floats and integers.  This struct gets
// instantiated when the "type" field of TOML element is the reserved name of a
// common scalar type, such as "uint16".
struct Terminal
{
    BOOST_HANA_DEFINE_STRUCT( Terminal,
    	( std::string, name ),
    	( std::streamoff, size )
    );
};

// Describe a single bit terminal type for boolean fields.  This struct gets
// instantiated when the the "type" field of a TOML element is "bit".
struct Bit
{
    BOOST_HANA_DEFINE_STRUCT( Bit,
        ( std::string, name )
    );

    static const std::uint8_t size = 1;
};

struct Bitset
{
    BOOST_HANA_DEFINE_STRUCT( Bitset,
        ( std::string, name ),
        ( std::uint8_t, size )
    );
};

// Describe a nested type embedded as an element of a higher level type.  The
// nested type must be registered separately in descriptor::catalog.  This
// struct gets instantiated when the "type" Field of a TOML element is not a
// native type key (like "uint16").
struct Nested
{
    BOOST_HANA_DEFINE_STRUCT( Nested,
    	( std::string, name )
    );
};

// Describe an arbitrary number of bytes to skip, useful for "reserved" space
// in a binary blob. This struct gets instantiated from the TOML "skip" element.
struct Skip
{
    BOOST_HANA_DEFINE_STRUCT( Skip,

    	// Remember how much extra space to skip or add back in, in bytes
        ( std::streamoff, size ),

	    // There can be multiple skips in a record, which need to be re-encoded
	    // back in order to preserve binary equivalence.  Maybe the vendors put
	    // something important there, but left it out of the documentation.  Keep a
	    // sort key for each skip, so the order can be preserved on re-encoding.
	    ( std::uint16_t, order )
    );
};

// Describe an arbitrary number of bits to skip, useful for "reserved" space
// that is not byte oriented (probably a CAN message).  This struct gets
// instantiated from the toml "bitskip" element.
struct BitSkip
{
    BOOST_HANA_DEFINE_STRUCT( BitSkip,
        ( std::uint8_t, size ),
        ( std::uint16_t, order )
    );

    const std::string name { "skip" };
};

// Describe an array of terminal or embedded types.  Arrays get complicated,
// because the size may either be a fixed constant in the type definition, or
// read from one of the parent node's Fields.  If the "count" Field in the TOML
// element is an *integer*, then the array will have fixed size.  If it is a
// *string*, then the string should equal one of the prior Fields in the record
// type that yields some type of integer.  This descriptor really covers four
// separate cases between fixed or dynamic size, and native or compound type.
struct Array
{
    BOOST_HANA_DEFINE_STRUCT( Array,

        // Fixed size, or Field name to look up.
        ( eggs::variant< size_t, std::string >, size ),

	    // Native type's typeid(), or the name of a compound type's description.
	    ( eggs::variant< std::type_index, std::string >, type )
    );
};

struct BitField
{
    using Type = eggs::variant< Bit, Bitset, BitSkip >;

    BOOST_HANA_DEFINE_STRUCT( BitField,
        ( std::vector<Type>, fields )
    );

    size_t size() const // total number of bits in field
    {
        return std::accumulate( fields.begin(), fields.end(), 0,
                []( size_t sum, const Type& var ) {
                    return sum + eggs::variants::apply([]( auto desc ) -> size_t { return desc.size; }, var );
                });
    }
};

inline bool operator==( const BitField::Type& lhs, const BitField::Type& rhs )
{
    return lhs == rhs;
}

inline bool operator!=( const BitField::Type& lhs, const BitField::Type& rhs )
{
    return !operator==( lhs, rhs );
}


struct Field
{
    std::string name;
    eggs::variant< std::type_index, Nested, Skip, Array, BitField > type;

    // Optional metadata about a Field may include bigendian storage, and a
    // specialized function to pretty print the value.  Using these attributes
    // is optional.
    descriptor::ByteOrder byteorder;

    std::function< std::string ( const polysync::Variant& ) > format;
};

template <typename T>
std::enable_if_t<hana::Struct<T>::value, bool>
operator==( const T& left, const T& right )
{
    return hana::equal( left, right );
}

inline bool operator==( const Field& lhs, const Field& rhs )
{
    return lhs.name == rhs.name and lhs.type == rhs.type;
}

inline bool operator!=( const Field& lhs, const Field& rhs )
{
    return !operator==( lhs, rhs );
}

inline bool operator==( const Type& lhs, const Type& rhs )
{
    return lhs.name == rhs.name
        and std::equal( lhs.begin(), lhs.end(), rhs.begin(), rhs.end(), hana::equal );
}

inline bool operator!=( const Type& lhs, const Type& rhs )
{
    return !operator==( lhs, rhs );
}

}} // namespace polysync::descriptor


