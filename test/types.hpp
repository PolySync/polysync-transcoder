#pragma once

#include <polysync/plog/core.hpp>
#include <polysync/descriptor.hpp>
#include <polysync/detector.hpp>

#include <boost/endian/arithmetic.hpp>
#include <boost/multiprecision/cpp_int.hpp>

namespace endian = boost::endian;
namespace hana = boost::hana;
namespace plog = polysync::plog;

constexpr auto integers = hana::tuple_t<
    std::int8_t, std::int16_t, std::int32_t, std::int64_t,
    std::uint8_t, std::uint16_t, std::uint32_t, std::uint64_t
    >;

constexpr auto reals = hana::tuple_t<float, double>;

const auto bigendians = hana::transform(integers, [](auto t) {
        using T = typename decltype(t)::type;
        return hana::type_c<endian::endian_arithmetic<endian::order::big, T, 8*sizeof(T)>>;
        });

auto scalars = hana::concat(integers, reals);

// Define metafunction to compute parameterized mettle::suite from a hana typelist.
template <typename TypeList>
using type_suite = typename decltype(
        hana::unpack(std::declval<TypeList>(), hana::template_<mettle::suite>)
        )::type;

// Teach mettle how to compare hana structures
template <typename Struct>
auto hana_equal(Struct&& expected) {

    return mettle::make_matcher(std::forward<Struct>(expected),
            [](const auto& actual, const auto& expected) -> bool {
            return hana::equal(actual, expected); },
            "hana_equal ");
}

namespace cpptoml {

// Teach mettle how to print TOML objects.  Must be in cpptoml namespace for
// discovery by argument dependent lookup.
template <typename T>
std::string to_printable(std::shared_ptr<T> p) {
    std::ostringstream stream;
    cpptoml::toml_writer writer { stream };
    p->accept(writer);
    return stream.str();
}

} // namespace cpptoml

struct has_key : mettle::matcher_tag {
    has_key(const std::string& key) : key(key) {}

    template <typename T, typename V>
    bool operator()(const std::map<T, V>& map) const {
        return map.count(key);
    }

    bool operator()(std::shared_ptr<cpptoml::base> base) const {
           if (!base->is_table())
               return false;
            return base->as_table()->contains(key);
    };

    std::string desc() const { return "has_key \"" + key + "\""; }
    const std::string key;
};

// Define a mettle matcher that knows how to compare an arbitarily long hex
// string with a bytes buffer.
auto bits(const std::string& hex) {
    namespace mp = boost::multiprecision;

    std::string truth;
    mp::export_bits(mp::cpp_int("0x" + hex), std::back_inserter(truth), 8);

    return mettle::make_matcher([truth](const auto& value) -> bool {
            return truth == value;
            }, hex);
}

namespace toml {

constexpr char const* ps_byte_array_msg = R"toml(
[ps_byte_array_msg]
    description = [
        { name = "dest_guid", type = "uint64" },
        { name = "data_type", type = "uint32" },
        { name = "payload", type = "uint32" }
    ]
    detector = { ibeo.header = { data_type = "160" } }
)toml";

constexpr char const* ibeo_header = R"toml(
[ibeo.header]
    description = [
        { name = "magic", type = ">uint32" },
        { name = "prev_size", type = ">uint32" },
        { name = "size", type = "uint8" },
        { skip = 4 },
        { name = "device_id", type = "uint8" },
        { name = "data_type", type = ">uint16", format = "hex" },
        { name = "time", "type" = ">NTP64" }
    ]
    detector = { ibeo.vehicle_state = { magic = "0xAFFEC0C2", data_type = "0x2807" }, ibeo.scan_data = { magic = "0xAFFEC0C2", data_type = "0x2205" } }
)toml";

} // namespace toml

namespace descriptor {

polysync::descriptor::type ps_byte_array_msg { "ps_byte_array_msg", {
        { "dest_guid", typeid(plog::guid) },
        { "data_type", typeid(uint32_t) },
        { "payload", typeid(uint32_t) } }
};

polysync::descriptor::type ibeo_header { "ibeo.header", {
    { "magic", typeid(std::uint32_t) },
    { "prev_size", typeid(std::uint32_t) },
    { "size", typeid(std::uint8_t) },
    { "skip-1", polysync::descriptor::skip { 1, 4 } },
    { "device_id", typeid(std::uint8_t) },
    { "data_type", typeid(std::uint16_t) },
    { "time", typeid(std::uint64_t) } }
};

}


