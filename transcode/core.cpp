#include <polysync/plog/core.hpp>
#include <polysync/plog/description.hpp>

namespace polysync { namespace plog {

std::map<plog::msg_type, std::string> type_support_map;

std::map<std::type_index, descriptor::terminal> descriptor::static_typemap {
    { std::type_index(typeid(std::int8_t)),  { "int8",  sizeof(std::int8_t) } },
    { std::type_index(typeid(std::int16_t)), { "int16", sizeof(std::int16_t) } },
    { std::type_index(typeid(std::int32_t)), { "int32", sizeof(std::int32_t) } },
    { std::type_index(typeid(std::int64_t)), { "int64", sizeof(std::int64_t) } },
    { std::type_index(typeid(std::uint8_t)), { "uint8", sizeof(std::uint8_t) } },
    { std::type_index(typeid(std::uint16_t)), { "uint16", sizeof(std::uint16_t) } },
    { std::type_index(typeid(std::uint32_t)), { "uint32", sizeof(std::uint32_t) } },
    { std::type_index(typeid(std::uint64_t)), { "uint64", sizeof(std::uint64_t) } },
    { std::type_index(typeid(msg_header)), { "msg_header", size<msg_header>::value() } },
    { std::type_index(typeid(log_record)), { "log_record", size<log_record>::value() } },
    { std::type_index(typeid(log_header)), { "log_header", size<log_header>::value() } },
    { std::type_index(typeid(sequence<std::uint32_t, log_module>)), 
        { "sequence<log_module>", size<sequence<std::uint32_t, log_module>>::value() } },
    { std::type_index(typeid(timestamp)), { "ps_timestamp", size<timestamp>::value() } },
    { std::type_index(typeid(sequence<std::uint32_t, type_support>)), 
        { "sequence<type_support>", size<sequence<std::uint32_t, type_support>>::value() } },
};

std::map<std::string, descriptor::terminal> descriptor::dynamic_typemap {
    { "int8",  { "int8",  sizeof(std::int8_t) } },
    { "int16", { "int16", sizeof(std::int16_t) } },
    { "int32", { "int32", sizeof(std::int32_t) } },
    { "int64", { "int64", sizeof(std::int64_t) } },
    { "uint8",  { "uint8",  sizeof(std::uint8_t) } },
    { "uint16", { "uint16", sizeof(std::uint16_t) } },
    { "uint32", { "uint32", sizeof(std::uint32_t) } },
    { "uint64", { "uint64", sizeof(std::uint64_t) } },
    { ">uint8",  { ">uint8",  sizeof(std::uint8_t) } },
    { ">uint16", { ">uint16", sizeof(std::uint16_t) } },
    { ">uint32", { ">uint32", sizeof(std::uint32_t) } },
    { ">uint64", { ">uint64", sizeof(std::uint64_t) } },
    { "ps_guid", { "ps_guid", sizeof(guid) } },
    { "ps_msg_type", { "ps_msg_type", sizeof(msg_type) } },
    { "log_record", { "log_record", size<log_record>::value() } },
    { "msg_header", { "msg_header", size<msg_header>::value() } },
    { "ps_timestamp", { "ps_timestamp", size<timestamp>::value() } },
};

}} // namespace polysync::plog
