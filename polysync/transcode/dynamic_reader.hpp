#pragma once

#include <polysync/transcode/reader.hpp>
#include <eggs/variant.hpp>
#include <boost/endian/buffers.hpp>

namespace polysync { namespace plog {

namespace endian = boost::endian;

struct tree;

// Ugh, eggs::variant lacks the recursive feature supported by the old
// boost::variant, which makes it a PITA to implement a syntax tree (no nested
// variants!).  The std::shared_ptr<tree> is a workaround for this eggs
// limitation.  Apparently we are getting std::variant in C++17; maybe we can
// factor out std::shared_ptr<tree> to just tree, then.
using variant = eggs::variant<
    std::shared_ptr<tree>,
    msg_header,
    float, double,
    std::int8_t, std::int16_t, int32_t, int64_t,
    std::uint8_t, std::uint16_t, uint32_t, uint64_t,
    endian::big_uint16_buf_t, endian::big_uint32_buf_t, endian::big_uint64_buf_t,
    endian::big_int16_buf_t, endian::big_int32_buf_t, endian::big_int64_buf_t,
    plog::sequence<std::uint32_t, std::uint8_t>
    >;

struct node : variant {
    using variant::variant;
    std::string name;
};

struct tree : std::map<std::string, node> {
    using std::map<std::string, node>::map;
};

struct compare {

    template <typename T, typename U>
    typename std::enable_if<std::is_convertible<T, U>::value, bool>::type
    operator()(const T& v1, const U& v2) { return v1 == v2; }

    template <typename T, typename U>
    typename std::enable_if<!std::is_convertible<T, U>::value, bool>::type
    operator()(const T&, const U&) { return false; }

    template <typename T, size_t N>
    bool operator()(const endian::endian_buffer<endian::order::big, T, N>& v1, const T& v2) { 
        return v2 == v1.value(); 
    }
};

template <typename T>
inline bool operator==(const node& a1, const T& a2) {
    static compare c;
    return eggs::variants::apply([a2](auto v1) { return c(v1, a2); }, a1);
}

struct dynamic_reader : reader {
    using reader::reader;
    node operator()(const std::string& type, std::shared_ptr<tree> parent);
    node operator()(std::streamoff off, const std::string& type, std::shared_ptr<tree> parent);
    node operator()();
    node operator()(const type_descriptor&);

};

}} // namespace polysync::plog
