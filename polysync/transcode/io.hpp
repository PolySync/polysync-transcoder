#pragma once

#include <polysync/transcode/core.hpp>
#include <polysync/transcode/console.hpp>
#include <polysync/transcode/dynamic_reader.hpp>

#include <sstream>
#include <string>

namespace polysync { namespace plog {

namespace hana = boost::hana;
using polysync::console::format;

template <typename T>
auto to_string(const T& record) -> decltype(std::to_string(record)) {
    return std::to_string(record);
}

inline auto to_string(const hash_type& hash) -> std::string {
    std::stringstream ss;
    ss << hash;
    return ss.str();
}

template <typename LenType, typename T>
inline auto to_string(const sequence<LenType, T>& seq) {
    std::stringstream ss;
    ss << seq;
    return ss.str();
}

template <typename T, int N>
inline auto to_string(const std::array<T, N>& seq) {
    std::stringstream ss;
    // ss << seq;
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
              << std::hex << pair.second;
}

inline std::ostream& operator<<(std::ostream& os, const plog::field_descriptor& desc) {
    return os << format.white << desc.type << ": " << format.blue << desc.name << format.normal;
}

inline std::ostream& operator<<(std::ostream& os, const plog::type_descriptor& desc) {
    os << "type_descriptor { ";
    std::for_each(desc.begin(), desc.end(), [&os](auto field) { os << field << ", "; });
    return os << "}";
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
    os << "[ " << std::hex;
    std::for_each(record.begin(), record.end(), [&os](auto field) mutable { os << (std::uint16_t)field << " "; });
    os << " ]" << std::dec;
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

template <typename T>
auto to_string(const T& record) -> std::string { 
    std::stringstream ss;
    ss << record;
    return ss.str();
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

inline std::ostream& operator<<(std::ostream& os, const tree& rec) {
    os << "{ ";
    std::for_each(rec.begin(), rec.end(), [&os](auto field) { os << field << ", "; });
    return os << "}";
}

inline std::ostream& operator<<(std::ostream& os, const std::shared_ptr<tree> tree) {
    return os << *tree;
}

inline std::ostream& operator<<(std::ostream& os, const plog::node& value) {
    eggs::variants::apply([&os](auto a) { os << a; }, value);
    return os;
}



}} // namespace polysync::plog


