#include <polysync/descriptor/catalog.hpp>

namespace polysync { namespace descriptor {

std::map< std::string, Type> catalog;

std::map<std::string, std::type_index> terminalNameMap
{
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

std::map<std::type_index, descriptor::Terminal> terminalTypeMap
{
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
};

}} // namespace polysync::descriptor
