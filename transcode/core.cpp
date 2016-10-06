#include <polysync/transcode/core.hpp>

namespace polysync { namespace plog {

std::vector<plog::field_descriptor> log_header {
    { "version_major", "uint8" },
    { "version_minor", "uint8" },
    { "version_subminor", "uint16" },
    { "build_date", "uint32" },
    { "node_guid", "uint64" },
    { "modules", "sequence<uint32>" },
    { "type_supports", "sequence<uint32>" }
};

std::map<std::type_index, plog::atom_description> static_typemap {
    { std::type_index(typeid(std::uint8_t)), { "uint8", sizeof(std::uint8_t) } },
    { std::type_index(typeid(std::uint16_t)), { "uint16", sizeof(std::uint16_t) } },
    { std::type_index(typeid(std::uint32_t)), { "uint32", sizeof(std::uint32_t) } },
    { std::type_index(typeid(std::uint64_t)), { "uint64", sizeof(std::uint64_t) } },
    { std::type_index(typeid(msg_header)), { "msg_header", size<msg_header>::packed() } },
    { std::type_index(typeid(log_record)), { "log_record", size<log_record>::packed() } },
};

std::map<std::string, plog::atom_description> dynamic_typemap {
    { "uint8", { "uint8", sizeof(std::uint8_t) } },
    { "uint16", { "uint16", sizeof(std::uint16_t) } },
    { "uint32", { "uint32", sizeof(std::uint32_t) } },
    { "uint64", { "uint64", sizeof(std::uint64_t) } },
    { "ps_guid", { "ps_guid", sizeof(ps_guid) } },
    { "ps_msg_type", { "ps_msg_type", sizeof(ps_msg_type) } },
    { "msg_header", { "msg_header", size<msg_header>::packed() } },
};

}} // namespace polysync::plog
