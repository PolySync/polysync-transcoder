#include <polysync/transcode/variant.hpp>
#include <polysync/transcode/io.hpp>
#include <iostream>
#include <mettle.hpp>

using namespace mettle;
namespace endian = boost::endian;
namespace plog = polysync::plog;

struct number_factory {
    template <typename T>
    plog::variant make() { return static_cast<T>(42); }
};

// Check variant equality over each number type
mettle::suite<
        std::uint8_t, std::uint16_t, std::uint32_t, std::uint64_t, 
        std::int8_t, std::int16_t, std::int32_t, std::int64_t,
        float, double
    > native("native", number_factory{}, [](auto& suite) {
            using T = mettle::fixture_type_t<decltype(suite)>;

            suite.test("equal", [](plog::variant var) { 
                    expect(var, equal_to(T { 42 })); 
                    });
            
            suite.test("not_equal", [](plog::variant var) { 
                    expect(var, not_equal_to(T { 41 })); 
                    });

            suite.test("type_mismatch", [](plog::variant var) { 
                    expect(var, not_equal_to(endian::big_int16_t { 42 })); 
                    });
        });

// Check variant equality over each endian swapped type
mettle::suite<
        endian::big_uint16_t, endian::big_uint32_t, endian::big_uint64_t,
        endian::big_int16_t, endian::big_int32_t, endian::big_int64_t
    > bigendian("bigendian", number_factory{}, [](auto& suite) {
            using T = mettle::fixture_type_t<decltype(suite)>;

            suite.test("equal", [](plog::variant var) { 
                    expect(var, equal_to(T { 42 })); 
                    });

            suite.test("not_equal", [](plog::variant var) { 
                    expect(var, not_equal_to(T { 41 })); 
                    });

            suite.test("type_mismatch", [](plog::variant var) { 
                    expect(var, not_equal_to(float { 42 })); 
                    });

            suite.test("endian_swap", [](plog::variant var) { 
                    expect(var, equal_to(typename T::value_type { 42 })); 
                    });
        });

// Check variant == variant over integer types
mettle::suite<
        std::uint8_t, std::uint16_t, std::uint32_t, std::uint64_t, 
        std::int8_t, std::int16_t, std::int32_t, std::int64_t,
        float, double
        > variant_eq("variant_eq", number_factory{}, [](auto& suite) {
            using T = mettle::fixture_type_t<decltype(suite)>;

            suite.test("equal", [](plog::variant var) {
                    expect(var, equal_to(plog::variant(T { 42 })));
                    });

            suite.test("not_equal", [](plog::variant var) {
                    expect(var, not_equal_to(plog::variant(T { 41 })));
                });

            });

// Check variant == variant over bigendian types
mettle::suite<
        std::uint8_t, std::uint16_t, std::uint32_t, std::uint64_t, 
        std::int8_t, std::int16_t, std::int32_t, std::int64_t
        > variant_bigendian_eq("variant_bigendian_eq", number_factory{}, [](auto& suite) {
            using T = mettle::fixture_type_t<decltype(suite)>;
            using bigendian = endian::endian_arithmetic<endian::order::big, T, 8*sizeof(T)>;

            suite.test("equal", [](plog::variant var) {
                    expect(var, equal_to(plog::variant(bigendian { 42 })));
                    });

            suite.test("not_equal", [](plog::variant var) {
                    expect(var, not_equal_to(plog::variant(bigendian { 41 })));
                });

            });

