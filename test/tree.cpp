#include <polysync/tree.hpp>
#include <polysync/io.hpp>
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
                expect(var, not_equal_to(std::vector<std::uint8_t>{ 42 })); 
                });
        });

// Check variant equality over each endian swapped type
// type_suite<decltype(bigendians)>
// bigendian("plog::variant == boost::endian", number_factory {}, [](auto& _) {
//             using T = mettle::fixture_type_t<decltype(_)>;
// 
//             _.test("equal", [](plog::variant var) { 
//                     expect(var, equal_to(T { 42 })); 
//                     });
// 
//             _.test("not_equal", [](plog::variant var) { 
//                     expect(var, not_equal_to(T { 41 })); 
//                     });
// 
//             _.test("type_mismatch", [](plog::variant var) { 
//                     expect(var, not_equal_to(float { 42 })); 
//                     });
// 
//             _.test("endian_swap", [](plog::variant var) { 
//                     expect(var, equal_to(typename T::value_type { 42 })); 
//                     });
//         });
// 
// // Check variant == variant over integer and bigendian types
// type_suite<decltype(hana::concat(integers, bigendians))>
// variant_eq("plog::variant == plog::variant", number_factory{}, [](auto& _) {
//             using T = mettle::fixture_type_t<decltype(_)>;
// 
//             _.test("equal", [](plog::variant var) {
//                     expect(var, equal_to(plog::variant(T { 42 })));
//                     });
// 
//             _.test("not_equal", [](plog::variant var) {
//                     expect(var, not_equal_to(plog::variant(T { 41 })));
//                 });
// 
//             });

mettle::suite<> terminal_array("terminal_array", [](auto& _) {

        _.test("equal", []() {
                plog::node truth("array", std::vector<std::uint8_t> { 1, 2, 3 } );
                plog::node correct("array", std::vector<std::uint8_t> { 1, 2, 3 } );
                expect(correct, equal_to(truth));
                });

        _.test("notequal", []() {
                plog::node truth("array", std::vector<std::uint8_t> { 1, 2, 3 } );
                plog::node wrong("array", std::vector<std::uint8_t> { 1, 2, 4 } );
                expect(wrong, not_equal_to(truth));
                });

        _.test("tooshort", []() {
                plog::node truth("array", std::vector<std::uint8_t> { 1, 2, 3 } );
                plog::node tooshort("array", std::vector<std::uint8_t> { 1, 2 } );
                expect(tooshort, not_equal_to(truth));
                });

        _.test("toolong", []() {
                plog::node truth("array", std::vector<std::uint8_t> { 1, 2, 3 });
                plog::node toolong("array", std::vector<std::uint8_t> { 1, 2, 3, 4 });
                expect(toolong, not_equal_to(truth));
                });
});

mettle::suite<> tree("tree", [](auto& _) {

        _.test("equal", []() {
                plog::tree equal = plog::tree::create({
                        plog::tree::create({
                                { "start_time", std::uint16_t { 1 } },
                                { "scanner_count", std::uint32_t { 2 } }
                                })
                        });

                plog::tree truth = plog::tree::create({
                        plog::tree::create({
                                { "start_time", std::uint16_t { 1 } },
                                { "scanner_count", std::uint32_t { 2 } }
                                })
                        });

                expect(equal, equal_to(truth));
                });

        _.test("notequal", []() {
                plog::tree notequal = plog::tree::create({
                        plog::tree::create({
                                { "start_time", std::uint16_t { 1 } },
                                { "scanner_count", std::uint32_t { 3 } }
                                })
                        });

                plog::tree truth = plog::tree::create({
                        plog::tree::create({
                                { "start_time", std::uint16_t { 1 } },
                                { "scanner_count", std::uint32_t { 2 } }
                                })
                        });

                expect(notequal, not_equal_to(truth));
                });

        _.test("tooshort", []() {
                plog::tree tooshort = plog::tree::create({
                        plog::tree::create({
                                { "start_time", std::uint16_t { 1 } },
                                })
                        });

                plog::tree truth = plog::tree::create({
                        plog::tree::create({
                                { "start_time", std::uint16_t { 1 } },
                                { "scanner_count", std::uint32_t { 2 } }
                                })
                        });

                expect(tooshort, not_equal_to(truth));
                });

        _.test("toolong", []() {
                plog::tree toolong = plog::tree::create({
                        plog::tree::create({
                                { "start_time", std::uint16_t { 1 } },
                                { "scanner_count", std::uint32_t { 2 } },
                                { "stop_time", std::uint16_t { 3 } },
                                })
                        });

                plog::tree truth = plog::tree::create({
                        plog::tree::create({
                                { "start_time", std::uint16_t { 1 } },
                                { "scanner_count", std::uint32_t { 2 } }
                                })
                        });

                expect(toolong, not_equal_to(truth));
                });
});

mettle::suite<> nested_tree("nested_tree", [](auto& _) {

        _.test("equal", []() {
                plog::tree equal = plog::tree::create({
                        { "header", std::uint16_t { 1 } },
                        plog::tree::create({
                                { "start_time", std::uint16_t { 2 } },
                                { "scanner_count", std::uint32_t { 3 } }
                                })
                        });

                plog::tree truth = plog::tree::create({
                        { "header", std::uint16_t { 1 } },
                        plog::tree::create({
                                { "start_time", std::uint16_t { 2 } },
                                { "scanner_count", std::uint32_t { 3 } }
                                })
                        });

                expect(equal, equal_to(truth));
                });

        _.test("notequal", []() {
                plog::tree notequal = plog::tree::create({
                        { "header", std::uint16_t { 1 } },
                        plog::tree::create({
                                { "start_time", std::uint16_t { 2 } },
                                { "scanner_count", std::uint32_t { 4 } }
                                })
                        });

                plog::tree truth = plog::tree::create({
                        { "header", std::uint16_t { 1 } },
                        plog::tree::create({
                                { "start_time", std::uint16_t { 2 } },
                                { "scanner_count", std::uint32_t { 3 } }
                                })
                        });

                expect(notequal, not_equal_to(truth));
                });

        _.test("tooshort", []() {
                plog::tree tooshort = plog::tree::create({
                        { "header", std::uint16_t { 1 } },
                        plog::tree::create({
                                { "start_time", std::uint16_t { 2 } },
                                })
                        });

                plog::tree truth = plog::tree::create({
                        { "header", std::uint16_t { 1 } },
                        plog::tree::create({
                                { "start_time", std::uint16_t { 2 } },
                                { "scanner_count", std::uint32_t { 3 } }
                                })
                        });

                expect(tooshort, not_equal_to(truth));
                });

        _.test("toolong", []() {
                plog::tree toolong = plog::tree::create({
                        { "header", std::uint16_t { 1 } },
                        plog::tree::create({
                                { "start_time", std::uint16_t { 2 } },
                                { "scanner_count", std::uint32_t { 3 } },
                                { "stop_time", std::uint16_t { 4 } },
                                })
                        });

                plog::tree truth = plog::tree::create({
                        { "header", std::uint16_t { 1 } },
                        plog::tree::create({
                                { "start_time", std::uint16_t { 2 } },
                                { "scanner_count", std::uint32_t { 3 } }
                                })
                        });

                expect(toolong, not_equal_to(truth));
                });
});


