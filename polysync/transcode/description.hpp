#pragma once

#include <boost/hana.hpp>

namespace polysync { namespace plog {

namespace hana = boost::hana;

struct atom_description {
    std::string name;
    std::streamoff size;
};

extern std::map<std::type_index, atom_description> static_typemap; 
extern std::map<std::string, atom_description> dynamic_typemap; 

template <typename Number, class Enable = void>
struct size {
    static std::streamoff packed() { return sizeof(Number); }
};

template <typename Struct>
struct size<Struct, typename std::enable_if<hana::Foldable<Struct>::value>::type> {
    static std::streamoff packed() {
        return hana::fold(hana::members(Struct()), 0, [](std::streamoff s, auto field) { 
                return s + size<decltype(field)>::packed(); 
                });
    }
};

}} // namespace polysync::plog
// Hana adaptors must be in global namespace

BOOST_HANA_ADAPT_STRUCT(polysync::plog::log_header,
        version_major, version_minor, version_subminor, build_date, node_guid 
       , modules, type_supports
        );
BOOST_HANA_ADAPT_STRUCT(polysync::plog::log_module, version_major, version_minor, version_subminor, build_date
        , build_hash, name
         );
BOOST_HANA_ADAPT_STRUCT(polysync::plog::type_support, type, name);
BOOST_HANA_ADAPT_STRUCT(polysync::plog::log_record, index, size, prev_size, timestamp);
BOOST_HANA_ADAPT_STRUCT(polysync::plog::msg_header, type, timestamp, src_guid);

namespace polysync { namespace plog {

inline bool operator==(const msg_header&, const msg_header&) { return false; }
inline bool operator!=(const msg_header&, const msg_header&) { return true; }

}} // polysync::plog

