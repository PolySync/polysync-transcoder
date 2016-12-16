#pragma once

// Most PolySync and vendor specific message types are defined dynamically,
// using TOML strings embedded in the plog files, or external config files.
// Legacy types will be supported by a fallback in external files.  Ubiquitous
// message types are found in every plog file and described by boost::hana in 
// core.hpp, and not by the dynamic mechanism implemented here.

#include <typeindex>

#include <boost/hana.hpp>

#include <deps/cpptoml.h>

#include <polysync/console.hpp>
#include <polysync/tree.hpp>
#include <polysync/exception.hpp>
#include <polysync/print_tree.hpp>

namespace polysync { namespace descriptor {

namespace hana = boost::hana;

// A node may contain a terminal type, a nested compound type, an array of
// nested or native types, or a skip (to implement padding or reserved space).
struct terminal {
    std::string name;
    std::streamoff size;
};

struct nested {
    // Name the compound type of the embedded value
    std::string name;
};

struct skip {
    // Keep the order of the skips, so they can go back in order
    std::uint16_t order;
    // Remember how much extra space to skip or add back in, in bytes.
    std::streamoff size;
};

// Arrays get complicated, because the size may either be fixed in the type
// definition, or read from one of the parent node's fields.  Furthermore, the
// value of the array may be either a native type, or a compound type.
struct array {
    // Fixed size, or field name to look up.
    eggs::variant<size_t, std::string> size;

    // Native type's typeid(), or the name of a compound type's description.
    eggs::variant<std::type_index, std::string> type;
};


// The unit tests require comparing these classes.
inline bool operator==( const terminal& lhs, const terminal& rhs ) {
    return lhs.name == rhs.name && lhs.size == rhs.size;
}

inline bool operator==( const nested& lhs, const nested& rhs ) {
    return lhs.name == rhs.name;
}

inline bool operator==( const skip& lhs, const skip& rhs ) {
    return lhs.order == rhs.order && lhs.size == rhs.size;
}

inline bool operator==( const array& lhs, const array& rhs ) {
    return lhs.size == rhs.size && lhs.type == rhs.type;
}

struct field {
    using variant = eggs::variant<std::type_index, nested, skip, array>;

    // The field must know it's name, and the type.
    std::string name;
    variant type;

    // Optional metadata about a field may include bigendian storage, and a
    // specialized function to pretty print the value.  Using these attributes
    // is optional.
    bool bigendian { false };

    std::function<std::string ( const polysync::variant& )> format {
        // The default implementation just finds the operator<< overload for the node.
        [](const polysync::variant& n) {
            std::stringstream os;
            eggs::variants::apply([&os]( auto v ) { os << v; }, n);
            return os.str();
        }};

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

using catalog_type = std::map<std::string, type>;

// Global type descriptor catalogs
extern catalog_type catalog;
extern std::map<std::type_index, terminal> typemap;
extern std::map<std::string, std::type_index> namemap;

// Traverse a TOML table
extern void load( const std::string& name, std::shared_ptr<cpptoml::table> table, catalog_type& );

// Create a type description of a static structure, using hana for class instrospection
template <typename Struct>
struct describe {

    // Static introspection of the descriptor for Struct
    static descriptor::type type() {
        namespace hana = boost::hana;
        if ( !typemap.count( typeid( Struct ) ) )
            throw polysync::error( "unnamed type" )
                << exception::type( typeid( Struct ).name() )
                << exception::module( "description" )
                << status::description_error;

        std::string tpname = descriptor::typemap.at( typeid( Struct ) ).name;

        return hana::fold( Struct(), descriptor::type(tpname), []( auto desc, auto pair ) {
                std::string name = hana::to<char const*>( hana::first( pair ) );

                if ( typemap.count( typeid( hana::second( pair ) ) ) == 0)
                    throw polysync::error( "missing typemap" )
                        << exception::module( "description" )
                        << exception::type( name )
                        << status::description_error;

                terminal a = typemap.at( std::type_index( typeid( hana::second(pair) ) ) );
                desc.emplace_back(field { name, typeid( hana::second( pair ) ) } );
                return desc;
                });
    }

    // Generate self descriptions of types
    static std::string string() {
        if ( !descriptor::typemap.count( typeid(Struct) ) )
            throw polysync::error( "no typemap description" )
                        << exception::module( "description" )
                        << exception::type( typeid(Struct).name() )
                        << status::description_error;

        std::string tpname = descriptor::typemap.at( typeid(Struct) ).name;
        std::string result = tpname + " { ";
        hana::for_each(Struct(), [ &result, tpname ]( auto pair ) {
                std::type_index tp = typeid(hana::second( pair ));
                std::string fieldname = hana::to<char const*>( hana::first(pair) );
                if ( descriptor::typemap.count(tp) == 0 )
                    throw polysync::error( "type not described" )
                        << exception::type( tpname )
                        << exception::field( fieldname )
                        << exception::module( "description" )
                        << status::description_error;

                result += fieldname + ": " + descriptor::typemap.at(tp).name + "; ";
                });
        return result + "}";
    }

};

struct lex : std::string {

    lex( const field::variant& v );
    std::string operator()( std::type_index ) const;
    std::string operator()( nested ) const;
    std::string operator()( skip ) const;
    std::string operator()( array ) const;
    std::string operator()( std::string ) const;
    std::string operator()( size_t ) const;

};

// Unit tests, exception handling, and the logger all require printing the type
// descriptions to the console.

extern std::ostream& operator<<(std::ostream& os, const field& f);
extern std::ostream& operator<<(std::ostream& os, const type& desc);

}} // namespace polysync::descriptor
