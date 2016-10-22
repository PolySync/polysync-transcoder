#pragma once

#include <polysync/transcode/reader.hpp>
#include <polysync/transcode/variant.hpp>
#include <polysync/transcode/description.hpp>

namespace polysync { namespace plog {

// Dynamic parsing builds a tree of nodes to represent the record.  Each leaf
// is strongly typed as one of the variant component types.
struct node : variant {
    using variant::variant;
    std::string name;
};

struct tree : std::map<std::string, node> {
    using std::map<std::string, node>::map;
    std::string name;
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

struct dynamic_reader : reader {
    using reader::reader;
    node operator()(const std::string& type, std::shared_ptr<tree> parent);
    node operator()(std::streamoff off, const std::string& type, std::shared_ptr<tree> parent);
    node operator()();
    std::shared_ptr<tree> operator()(const std::string&, const type_descriptor&);

    size_t order { 1 };
};

}} // namespace polysync::plog
