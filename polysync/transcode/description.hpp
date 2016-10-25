#pragma once

// Most PolySync and vendor specific message types are defined dynamically,
// using TOML strings embedded in the plog files, or external config files.
// Legacy types will be supported by a fallback in external files.  Ubiquitous
// message types are found in every plog file, and are specially defined in
// core.hpp, and not by the dynamic mechanism implemented here.

#include <polysync/transcode/variant.hpp>
#include <polysync/transcode/size.hpp>
#include <polysync/3rdparty/cpptoml.h>

#include <boost/hana.hpp>
#include <typeindex>

namespace polysync { namespace plog {

namespace descriptor {

struct field {
    std::string name;
    std::string type;
};

// The full type description is just a vector of fields
using type = std::vector<field>;

struct atom {
    std::string name;
    std::streamoff size;
};

// Traverse a TOML table to install a new type description in the global catalog
extern void load(const std::string& name, std::shared_ptr<cpptoml::table> table);

// Global type descriptor catalogs
extern std::map<std::string, type> catalog;
extern std::map<std::type_index, atom> static_typemap; 
extern std::map<std::string, atom> dynamic_typemap; 

// Create a type description of a static structure, using hana for class instrospection
template <typename Struct>
inline type describe() {
    namespace hana = boost::hana;

    return hana::fold(Struct(), type(), [](auto desc, auto pair) { 
            std::string name = hana::to<char const*>(hana::first(pair));
            if (static_typemap.count(typeid(hana::second(pair))) == 0)
                throw std::runtime_error("missing typemap for " + name);
            atom a = static_typemap.at(typeid(hana::second(pair)));
            desc.emplace_back(field { name, a.name });
            return desc;
            });
}

} // namespace descriptor

template <>
struct size<descriptor::field> {
    size(const descriptor::field& f) : desc(f) { }
    
    std::streamoff value() const {
        return descriptor::dynamic_typemap.at(desc.type).size;
    }

    descriptor::field desc;
};


namespace detector {

struct type {
    std::string parent;
    std::map<std::string, variant> match;
    std::string child;
};

extern void load(const std::string& name, std::shared_ptr<cpptoml::table> table);

extern std::vector<type> catalog;

} // detector


}} // namespace polysync::plog


