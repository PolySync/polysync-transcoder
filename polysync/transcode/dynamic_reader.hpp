#pragma once

#include <polysync/transcode/reader.hpp>
#include <polysync/transcode/variant.hpp>
#include <polysync/transcode/description.hpp>

namespace polysync { namespace plog {

// Dynamic parsing builds a tree of nodes to represent the record.  Each leaf
// is strongly typed as one of the variant component types.
struct node : variant {

    template <typename T>
    node(const T& value, std::string n) : variant(value), name(n) { }

    using variant::variant;
    std::string name;

    template <typename Struct>
    static node from(const Struct&);
 };

struct tree : std::vector<node> {};

template <typename Struct>
inline node node::from(const Struct& s) {
    std::shared_ptr<tree> tr = std::make_shared<tree>();
    hana::for_each(s, [tr](auto pair) { 
            tr->emplace_back(hana::second(pair), hana::to<char const*>(hana::first(pair)));
            });
        
    return node(tr);
}

struct dynamic_reader : reader {
    using reader::reader;
    using reader::read;

    node read();
    node read(const std::string& type, const std::vector<node>&);
    // node read(const std::string&, const descriptor::type&);
};

}} // namespace polysync::plog
