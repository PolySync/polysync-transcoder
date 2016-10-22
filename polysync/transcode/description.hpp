#pragma once

// Most PolySync message types are defined dynamically, typically as strings
// located in the plog files themselves.  Legacy types will be supported by a
// fallback mechanism (also defined as strings).  Select message types are
// found in every plog file, and are specially defined in core.hpp, and not by
// the dynamic mechanism implemented here.

#include <polysync/transcode/variant.hpp>
#include <polysync/transcode/size.hpp>
#include <polysync/3rdparty/cpptoml.h>

#include <boost/hana.hpp>
#include <typeindex>

namespace polysync { namespace plog {

extern void load_description(const std::string& name, std::shared_ptr<cpptoml::table> table);
extern void load_detector(const std::string& name, std::shared_ptr<cpptoml::table> table);

struct detector_type {
    std::string parent;
    std::map<std::string, variant> match;
    std::string child;
};

extern std::vector<detector_type> detector_list;

namespace hana = boost::hana;

struct atom_description {
    std::string name;
    std::streamoff size;
};

struct field_descriptor {
    std::string name;
    std::string type;
};

using type_descriptor = std::vector<field_descriptor>;

extern std::map<std::string, type_descriptor> description_map;
extern std::map<std::type_index, atom_description> static_typemap; 
extern std::map<std::string, atom_description> dynamic_typemap; 

template <>
struct size<field_descriptor> {
    size(const field_descriptor& f) : field(f) { }
    
    std::streamoff value() const {
        return dynamic_typemap.at(field.type).size;
    }

    field_descriptor field;
};

}} // namespace polysync::plog


