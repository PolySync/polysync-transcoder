#pragma once

#include <polysync/transcode/reader.hpp>
#include <polysync/transcode/variant.hpp>
#include <polysync/transcode/description.hpp>

namespace polysync { namespace plog {

struct field_descriptor {
    std::string name;
    std::string type;
};

struct detector_type {
    std::string parent;
    std::map<std::string, variant> match;
    std::string child;
};

using type_descriptor = std::vector<field_descriptor>;

extern std::map<std::string, type_descriptor> description_map;
extern std::vector<detector_type> detector_list;

template <>
struct size<field_descriptor> {
    size(const field_descriptor& f) : field(f) { }

    std::streamoff packed() {
        return dynamic_typemap.at(field.type).size;
    }

    field_descriptor field;
};

template <typename Struct>
inline type_descriptor describe() {
    return hana::fold(Struct(), type_descriptor(), [](auto desc, auto pair) { 
            std::string name = hana::to<char const*>(hana::first(pair));
            if (static_typemap.count(typeid(hana::second(pair))) == 0)
                throw std::runtime_error("missing typemap for " + name);
            plog::atom_description atom = static_typemap.at(typeid(hana::second(pair)));
            desc.emplace_back(field_descriptor { name, atom.name });
            return desc;
            });
}


struct node : variant {
    using variant::variant;
    std::string name;
};


struct tree : std::map<std::string, node> {
    using std::map<std::string, node>::map;
    std::string name;
};

struct dynamic_reader : reader {
    using reader::reader;
    node operator()(const std::string& type, std::shared_ptr<tree> parent);
    node operator()(std::streamoff off, const std::string& type, std::shared_ptr<tree> parent);
    node operator()();
    std::shared_ptr<tree> operator()(const std::string&, const type_descriptor&);

    size_t order { 1 };
};

}} // namespace polysync::plog
