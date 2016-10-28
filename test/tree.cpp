#include <polysync/plog/tree.hpp>
#include <polysync/plog/io.hpp>
#include <iostream>
#include <mettle.hpp>
#include <boost/hana/ext/std/integral_constant.hpp>
#include "types.hpp"

using namespace mettle;
namespace endian = boost::endian;
namespace plog = polysync::plog;
namespace hana = boost::hana;

struct number_factory {
    template <typename T>
    plog::variant make() { return static_cast<T>(42); }
};

type_suite<decltype(hana::concat(integers, reals))> 
number("plog::variant == number", number_factory {}, [](auto& _) {

        using T = mettle::fixture_type_t<decltype(_)>;

        _.test("equal", [](plog::variant var) { 
                expect(var, equal_to(T { 42 })); 
                });

        _.test("not_equal", [](plog::variant var) { 
                expect(var, not_equal_to(T { 41 })); 
                });

        _.test("type_mismatch", [](plog::variant var) { 
                expect(var, not_equal_to(endian::big_int16_t { 42 })); 
                });
        });

// Check variant equality over each endian swapped type
type_suite<decltype(bigendians)>
bigendian("plog::variant == boost::endian", number_factory {}, [](auto& _) {
            using T = mettle::fixture_type_t<decltype(_)>;

            _.test("equal", [](plog::variant var) { 
                    expect(var, equal_to(T { 42 })); 
                    });

            _.test("not_equal", [](plog::variant var) { 
                    expect(var, not_equal_to(T { 41 })); 
                    });

            _.test("type_mismatch", [](plog::variant var) { 
                    expect(var, not_equal_to(float { 42 })); 
                    });

            _.test("endian_swap", [](plog::variant var) { 
                    expect(var, equal_to(typename T::value_type { 42 })); 
                    });
        });

// Check variant == variant over integer and bigendian types
type_suite<decltype(hana::concat(integers, bigendians))>
variant_eq("plog::variant == plog::variant", number_factory{}, [](auto& _) {
            using T = mettle::fixture_type_t<decltype(_)>;

            _.test("equal", [](plog::variant var) {
                    expect(var, equal_to(plog::variant(T { 42 })));
                    });

            _.test("not_equal", [](plog::variant var) {
                    expect(var, not_equal_to(plog::variant(T { 41 })));
                });

            });

