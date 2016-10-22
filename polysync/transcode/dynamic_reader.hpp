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

struct dynamic_reader : reader {
    using reader::reader;
    node operator()(const std::string& type, std::shared_ptr<tree> parent);
    node operator()(std::streamoff off, const std::string& type, std::shared_ptr<tree> parent);
    node operator()();
    std::shared_ptr<tree> operator()(const std::string&, const descriptor::type&);

    size_t order { 1 };
};

}} // namespace polysync::plog
