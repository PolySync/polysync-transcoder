#pragma once

#include "core.hpp"

#include <sstream>
#include <string>

namespace polysync { namespace plog {

namespace hana = boost::hana;

template <typename T>
auto to_string(const T& record) -> decltype(std::to_string(record)) {
    return std::to_string(record);
}

template <typename T>
auto to_string(const std::uint8_t record) -> decltype(std::to_string(record)) {
    return std::to_string(record);
}

inline auto to_string(const hash_type& hash) -> std::string {
    std::stringstream ss;
    ss << hash;
    return ss.str();
}

inline auto to_string(const msg_header& record) -> std::string {
    std::stringstream ss;
    // ss << record;
    return ss.str();
}


template <typename LenType, typename T>
auto to_string(const sequence<LenType, T>& seq) {
    std::stringstream ss;
    ss << seq;
    return ss.str();
}


template <typename T, typename U>
std::ostream& operator<<(std::ostream& os, const hana::pair<T, U>& pair) {
    return os << hana::to<char const*>(hana::first(pair)) << ": " << to_string(hana::second(pair));
}

// Specialize the std::uint8_t case so it does not print an ASCII character, but rather an int
template <typename T>
std::ostream& operator<<(std::ostream& os, const hana::pair<T, std::uint8_t>& pair) {
    return os << hana::to<char const*>(hana::first(pair)) << ": " << std::to_string(hana::second(pair)) << ";";
}

template <typename LenType, typename T>
std::ostream& operator<<(std::ostream& os, const sequence<LenType, T>& record) {
    os << "[ ";
    std::for_each(record.begin(), record.end(), [&os](auto field) mutable { os << "\n\t" << field; });
    os << " ]";
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

inline std::ostream& operator<<(std::ostream& os, const byte_array& record) {
    auto f = [](std::ostream& os, auto field) mutable -> std::ostream& { return os << field << " "; };
    return hana::fold(record, os, f);
}

template <typename T>
auto to_string(const T& record) -> std::string { 
    std::stringstream ss;
    ss << record;
    return ss.str();
}

inline std::ostream& operator<<(std::ostream& os, const log_header& record) {
    auto f = [](std::ostream& os, auto field) mutable -> std::ostream& { return os << field << std::endl; };
    return hana::fold(record, os, f);
}

inline std::ostream& operator<<(std::ostream& os, const log_record& record) {
    auto f = [](std::ostream& os, auto field) mutable -> std::ostream& { return os << field << std::endl; };
    return hana::fold(record, os, f);
}

// inline std::ostream& operator<<(std::ostream& os, const msg_header& record) {
//     auto f = [](std::ostream& os, auto field) mutable -> std::ostream& { return os << field << std::endl; };
//     return hana::fold(record, os, f);
// }

inline std::ostream& operator<<(std::ostream& os, const msg_header& record) {
    os << "{ ";
    auto f = [](std::ostream& os, auto field) mutable -> std::ostream& { return os << field << " "; };
    hana::fold(record, os, f);
    return os << " }";
}


}} // namespace polysync::plog
