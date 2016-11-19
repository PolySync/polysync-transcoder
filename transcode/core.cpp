#include <polysync/plog/core.hpp>

namespace polysync { 

namespace plog {
std::map<plog::msg_type, std::string> type_support_map;
} 

namespace descriptor {

std::map<std::string, std::type_index> namemap {
    { "int8", typeid(std::int8_t) },
    { "int16", typeid(std::int16_t) },
    { "int32", typeid(std::int32_t) },
    { "int64", typeid(std::int64_t) },
    { "uint8",  typeid(std::uint8_t) },
    { "uint16", typeid(std::uint16_t) },
    { "uint32", typeid(std::uint32_t) },
    { "uint64", typeid(std::uint64_t) },
    { "float", typeid(float) },
    { "float32", typeid(float) },
    { "double", typeid(double) },
};

std::map<std::type_index, descriptor::terminal> typemap {
    { typeid(std::int8_t), { "int8", sizeof(std::int8_t) } },
    { typeid(std::int16_t), { "int16", sizeof(std::int16_t) } },
    { typeid(std::int32_t), { "int32", sizeof(std::int32_t) } },
    { typeid(std::int64_t), { "int64", sizeof(std::int64_t) } },
    { typeid(std::uint8_t), { "uint8", sizeof(std::uint8_t) } },
    { typeid(std::uint16_t), { "uint16", sizeof(std::uint16_t) } },
    { typeid(std::uint32_t), { "uint32", sizeof(std::uint32_t) } },
    { typeid(std::uint64_t), { "uint64", sizeof(std::uint64_t) } },
    { typeid(float), { "float", sizeof(float) } },
    { typeid(double), { "double", sizeof(double) } },
    
    { typeid(plog::msg_header), { "msg_header", size<plog::msg_header>::value() } },
    { typeid(plog::log_record), { "log_record", size<plog::log_record>::value() } },
    { typeid(plog::log_header), { "log_header", size<plog::log_header>::value() } },
    { typeid(plog::sequence<std::uint32_t, plog::log_module>), 
        { "sequence<log_module>", size<plog::sequence<std::uint32_t, plog::log_module>>::value() } },
    // { typeid(plog::timestamp), { "ps_timestamp", size<plog::timestamp>::value() } },
    { typeid(plog::sequence<std::uint32_t, plog::type_support>), 
        { "sequence<type_support>", size<plog::sequence<std::uint32_t, plog::type_support>>::value() }}
};

}} // namespace polysync::descriptor
