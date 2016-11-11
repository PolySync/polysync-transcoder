#pragma once

#include <polysync/plog/core.hpp>
#include <polysync/console.hpp>
#include <polysync/plog/decoder.hpp>

#include <sstream>
#include <string>

template <typename T>
std::ostream& operator<<(std::ostream& os, const std::vector<T>& record) {
    const int psize = 16;
    os << "[ " << std::hex;
    std::for_each(record.begin(), std::min(record.begin() + psize, record.end()), [&os](auto field) mutable { os << field << " "; });
    if (record.size() > 2*psize)
        os << "... ";
    if (record.size() > psize)
        std::for_each(record.end() - psize, record.end(), [&os](auto field) mutable { os << field << " "; });

    os << "]" << std::dec << " (" << record.size() << " elements)";
    return os;
}

inline std::ostream& operator<<(std::ostream& os, const std::vector<std::uint8_t>& record) {
    const int psize = 16;
    os << "[ " << std::hex;
    std::for_each(record.begin(), std::min(record.begin() + psize, record.end()), [&os](auto field) mutable { os << ((std::uint16_t)field & 0xFF) << " "; });
    if (record.size() > 2*psize)
        os << "... ";
    if (record.size() > psize)
        std::for_each(record.end() - psize, record.end(), [&os](auto field) mutable { os << ((std::uint16_t)field & 0xFF) << " "; });

    os << "]" << std::dec << " (" << record.size() << " elements)";
    return os;
}

namespace polysync { namespace plog {

namespace hana = boost::hana;
using polysync::console::format;

// std::ostream& operator<<(std::ostream& os, tree rec);

extern std::ostream& operator<<(std::ostream& os, const variant& v);

template <typename T>
auto to_string(const T& record) -> std::string { 
    std::stringstream ss;
    ss << record;
    return ss.str();
}

inline auto to_string(std::uint8_t val) -> std::string {
    std::stringstream ss;
    ss << (std::uint16_t)val;
    return ss.str();
}

template <typename T, typename U>
std::ostream& operator<<(std::ostream& os, const hana::pair<T, U>& pair) {
    return os << format.cyan << hana::to<char const*>(hana::first(pair)) << ": " 
        << format.normal << to_string(hana::second(pair));
}

template <typename U>
std::ostream& operator<<(std::ostream& os, const hana::pair<std::string, U>& pair) {
    return os << format.cyan << hana::first(pair) << ": " << format.normal
        << to_string(hana::second(pair));
}

template <typename T, typename U>
std::ostream& operator<<(std::ostream& os, const std::pair<T, U>& pair) {
    return os << format.cyan << pair.first << ": " << format.normal
        << std::hex << pair.second << std::dec;
}

// Specialize the std::uint8_t case so it does not print an ASCII character, but rather an int
template <typename T>
std::ostream& operator<<(std::ostream& os, const hana::pair<T, std::uint8_t>& pair) {
    return os << hana::to<char const*>(hana::first(pair)) << ": " << std::to_string(hana::second(pair)) << ";";
}

template <typename T, int N>
inline std::ostream& operator<<(std::ostream& os, const std::array<T, N>& record) {
    os << "[ ";
    std::for_each(record.begin(), record.end(), [&os](auto field) mutable { os << "\n\t" << field; });
    os << " ]";
    return os;
}

template <typename LenType, typename T>
inline std::ostream& operator<<(std::ostream& os, const sequence<LenType, T>& record) {
    os << "[ ";
    std::for_each(record.begin(), record.end(), [&os](auto field) mutable { os << "\n\t" << field; });
    os << " ]";
    return os;
}

template <>
inline std::ostream& operator<<(std::ostream& os, const sequence<std::uint32_t, std::uint8_t>& record) {
    const int psize = 16;
    os << "[ " << std::hex;
    std::for_each(record.begin(), std::min(record.begin() + psize, record.end()), 
            [&os](std::uint8_t value) mutable { os << ((std::uint16_t)value & 0xFF) << " "; });
    if (record.size() > 2*psize)
        os << "... ";
    if (record.size() > psize)
        std::for_each(record.end() - psize, record.end(), [&os](auto field) mutable { os << ((std::uint16_t)field & 0xFF) << " "; });

    os << "]" << std::dec << " (" << record.size() << " bytes)";
    return os;
}

template <>
inline std::ostream& operator<<(std::ostream& os, const sequence<std::uint16_t, std::uint8_t>& record) {
    return os << "\"" << static_cast<std::string>(record) << "\"";
}

inline std::ostream& operator<<(std::ostream& os, const log_module& record) {
    auto f = [](std::ostream& os, auto field) mutable -> std::ostream& { return os << field << " "; };
    return hana::fold(record, os, f);
}

inline std::ostream& operator<<(std::ostream& os, const type_support& record) {
    auto f = [](std::ostream& os, auto field) mutable -> std::ostream& { return os << field << " "; };
    return hana::fold(record, os, f);
}

inline std::ostream& operator<<(std::ostream& os, const log_header& record) {
    auto f = [](std::ostream& os, auto field) mutable -> std::ostream& { return os << field << " "; };
    return hana::fold(record, os, f);
}

inline std::ostream& operator<<(std::ostream& os, const msg_header& record) {
    os << "{ ";
    auto f = [](std::ostream& os, auto field) mutable -> std::ostream& { return os << field << ", "; };
    hana::fold(record, os, f);
    return os << " }";
}

inline auto to_string(const msg_header& record) -> std::string {
    std::stringstream ss;
    ss << record;
    return ss.str();
}

inline std::ostream& operator<<(std::ostream& os, const log_record& record) {
    auto f = [](std::ostream& os, auto field) mutable -> std::ostream& { return os << field << ", "; };
    os << "log_record { ";
    hana::fold(record, os, f);
    return os << "payload: " << record.blob.size() << " bytes }";
}

inline std::ostream& operator<<(std::ostream& os, std::uint8_t value) {
    return os << "value";
}

// inline std::ostream& operator<<(std::ostream& os, const plog::node& value) {
//     eggs::variants::apply([&os](auto a) { os << plog::to_string(a); }, value);
//     return os;
// }

// inline std::ostream& operator<<(std::ostream& os, tree rec) {
//     os << "{ ";
//     if (rec->size() > 0) {
//         std::for_each(rec->begin(), rec->end() - 1, [&os](auto node) { os << node << ", "; });
//         os << rec->back() << " ";  // Make the last one special so it does not print ",".
//     }
//     return os << "}";
// }

inline std::string lex(const variant& v) {
    std::stringstream os;
    eggs::variants::apply([&os](auto a) { os << a; }, v);
    return os.str();
}

namespace descriptor {

inline std::ostream& operator<<(std::ostream& os, const field& f) {
    const std::type_info& info = f.type.target_type();
    if (typemap.count(info))
        return os << format.white << typemap.at(info).name << ": " << format.blue << f.name << format.normal;
    else return os << format.blue << f.name << format.normal;
}

inline std::ostream& operator<<(std::ostream& os, const type& desc) {
    os << desc.name << ": { ";
    std::for_each(desc.begin(), desc.end(), [&os](auto f) { os << f << ", "; });
    return os << "}";
}

} // namespace descriptor

}} // namespace polysync::plog


