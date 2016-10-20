#pragma once

#include <boost/multiprecision/cpp_int.hpp>
#include <vector>
#include <cstdint>
#include <typeindex>
#include <map>

namespace polysync { namespace plog {

struct log_module;
struct type_support;

constexpr size_t PSYNC_MODULE_VERIFY_HASH_LEN = 16;

namespace multiprecision = boost::multiprecision;

using hash_type = multiprecision::number<multiprecision::cpp_int_backend<
    PSYNC_MODULE_VERIFY_HASH_LEN*8, 
    PSYNC_MODULE_VERIFY_HASH_LEN*8, 
    multiprecision::unsigned_magnitude>>;

struct payload : std::string {}; // vector<std::uint8_t> {};

// a sequence<LenType, T> is just a vector<T> that knows to read it's length as a LenType
template <typename LenType, typename T>
struct sequence : std::vector<T> {
    using length_type = LenType;
};

// specialize the std::uint8_t sequences because they are actually strings
template <typename LenType>
struct sequence<LenType, std::uint8_t> : std::string {
    using length_type = LenType;
};

using name_type = sequence<std::uint16_t, std::uint8_t>;
using msg_type = std::uint32_t;
using guid = std::uint64_t;
using timestamp = std::uint64_t;

struct log_module {
    std::uint8_t version_major;
    std::uint8_t version_minor;
    std::uint16_t version_subminor;
    std::uint32_t build_date;
    hash_type build_hash;
    name_type name;
};

struct type_support {
    std::uint32_t type;
    name_type name;
};
       
struct log_header {
    std::uint8_t version_major;
    std::uint8_t version_minor;
    std::uint16_t version_subminor;
    std::uint32_t build_date;
    std::uint64_t node_guid;
    sequence<std::uint32_t, log_module> modules;
    sequence<std::uint32_t, type_support> type_supports;
};

struct msg_header {
    msg_type type;
    plog::timestamp timestamp;
    guid src_guid;
};

// I put msg_header into log_header instead of the byte_array like PolySync
// core does.  This is because we must always branch on msg_header.type before
// the byte_array can be interpreted.  So this is a difference between
// transcode and PolySync core.
struct log_record {
    std::uint32_t index;
    std::uint32_t size;
    std::uint32_t prev_size;
    plog::timestamp timestamp;
    payload blob;
};

extern std::map<plog::msg_type, std::string> type_support_map;

}} // namespace polysync::plog


