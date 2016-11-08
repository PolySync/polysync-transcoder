#pragma once

// Most PolySync and vendor specific message types are defined dynamically,
// using TOML strings embedded in the plog files, or external config files.
// Legacy types will be supported by a fallback in external files.  Ubiquitous
// message types are found in every plog file, and are specially defined in
// core.hpp, and not by the dynamic mechanism implemented here.

#include <polysync/plog/tree.hpp>
#include <polysync/3rdparty/cpptoml.h>

#include <boost/hana.hpp>
#include <typeindex>

namespace polysync { namespace plog {

namespace descriptor {

struct terminal {
    std::string name;
    std::streamoff size;
};

inline bool operator==(const terminal& lhs, const terminal& rhs) {
    return lhs.name == rhs.name && lhs.size == rhs.size;
}

struct nested {
    std::string name;
};

inline bool operator==(const nested& lhs, const nested& rhs) {
    return lhs.name == rhs.name;
}


struct skip {
    std::streamoff size;
};

inline bool operator==(const skip& lhs, const skip& rhs) {
    return lhs.size == rhs.size;
}

struct terminal_array {
    std::size_t size;
    std::type_index type;
};

inline bool operator==(const terminal_array& lhs, const terminal_array& rhs) {
    return lhs.size == rhs.size;
}

struct type;

struct nested_array {
    std::size_t size;
    const type& desc;
};

inline bool operator==(const nested_array& lhs, const nested_array& rhs) {
    return lhs.size == rhs.size;
}


struct field {
    std::string name;
    eggs::variant<std::type_index, nested, skip, terminal_array, nested_array> type;
};

inline bool operator==(const field& lhs, const field& rhs) {
    return lhs.name == rhs.name && lhs.type == rhs.type;
}

// The full type description is just a vector of fields.  This has to be a
// vector, not a map, to preserve the serialization order in the plog flat file.
struct type : std::vector<field> {
    type(const std::string& n) : name(n) {}
    type(const char * n, std::initializer_list<field> init) : name(n), std::vector<field>(init) {}

    std::string name;
};

using catalog_type = std::map<std::string, type>;

// Traverse a TOML table 
extern void load(const std::string& name, std::shared_ptr<cpptoml::table> table, catalog_type&);

// Global type descriptor catalogs
extern catalog_type catalog;
extern std::map<std::type_index, terminal> static_typemap; 
extern std::map<std::string, terminal> dynamic_typemap; 
extern std::map<std::type_index, terminal> typemap;
extern std::map<std::string, std::type_index> namemap; 

// Create a type description of a static structure, using hana for class instrospection
template <typename Struct>
struct describe {

    static descriptor::type type() {
        namespace hana = boost::hana;

        return hana::fold(Struct(), type(), [](auto desc, auto pair) { 
                std::string name = hana::to<char const*>(hana::first(pair));
                if (static_typemap.count(typeid(hana::second(pair))) == 0)
                throw std::runtime_error("missing typemap for " + name);
                // terminal a = static_typemap.at(typeid(hana::second(pair)));
                // desc.emplace_back(field { name, typemap.at(a.name) });
                return desc;
                });
    }
        
    // Generate self descriptions of types 
    static std::string string() {
        if (!descriptor::static_typemap.count(typeid(Struct)))
            throw std::runtime_error("no typemap description");
        std::string tpname = descriptor::static_typemap.at(typeid(Struct)).name; 
        std::string result = tpname + " { ";
        hana::for_each(Struct(), [&result, tpname](auto pair) {
                std::type_index tp = typeid(hana::second(pair));
                std::string fieldname = hana::to<char const*>(hana::first(pair));
                if (descriptor::static_typemap.count(tp) == 0)
                    throw std::runtime_error("type not described for field \"" + tpname + "::" + fieldname + "\"");
                result += fieldname + ": " + descriptor::static_typemap.at(tp).name + " " + std::to_string(descriptor::static_typemap.at(tp).size) +  "; ";
                });
        return result + "}";
    }

};

} // namespace descriptor

// Define some metaprograms to compute the sizes of types.
template <typename Number, class Enable = void>
struct size {
    static std::streamoff value() { return sizeof(Number); }
};


template <typename Struct>
struct size<Struct, typename std::enable_if<hana::Foldable<Struct>::value>::type> {
    static std::streamoff value() {
        return hana::fold(hana::members(Struct()), 0, [](std::streamoff s, auto field) { 
                return s + size<decltype(field)>::value(); 
                });
    }
};


// template <>
// struct size<descriptor::field> {
//     size(const descriptor::field& f) : desc(f) { }
//     
//     std::streamoff value() const {
//         return descriptor::typemap.at(desc.type).size;
//     }
// 
//     descriptor::field desc;
// };

}} // namespace polysync::plog

BOOST_HANA_ADAPT_STRUCT(polysync::plog::descriptor::field, name, type);


