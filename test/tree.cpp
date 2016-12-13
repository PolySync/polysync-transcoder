#include <polysync/tree.hpp>
#include <polysync/print_hana.hpp>
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
    polysync::variant make() {
        return T { 42 };
    }
};

type_suite<decltype(hana::concat(integers, reals))>
number("polysync::variant == number", number_factory {}, [](auto& _) {

        using T = mettle::fixture_type_t<decltype(_)>;

        _.test("equal", [](polysync::variant var) {
                expect(var, equal_to(T { 42 }));
                });

        _.test("not_equal", [](polysync::variant var) {
                expect(var, not_equal_to(T { 41 }));
                });

        _.test("type_mismatch", [](polysync::variant var) {
                expect(var, not_equal_to(std::vector<std::uint8_t>{ 42 }));
                });
        });

mettle::suite<> terminal_array("terminal_array", [](auto& _) {

        _.test("equal", []() {
                polysync::node truth("array", std::vector<std::uint8_t> { 1, 2, 3 } );
                polysync::node correct("array", std::vector<std::uint8_t> { 1, 2, 3 } );
                expect(correct, equal_to(truth));
                });

        _.test("notequal", []() {
                polysync::node truth("array", std::vector<std::uint8_t> { 1, 2, 3 } );
                polysync::node wrong("array", std::vector<std::uint8_t> { 1, 2, 4 } );
                expect(wrong, not_equal_to(truth));
                });

        _.test("tooshort", []() {
                polysync::node truth("array", std::vector<std::uint8_t> { 1, 2, 3 } );
                polysync::node tooshort("array", std::vector<std::uint8_t> { 1, 2 } );
                expect(tooshort, not_equal_to(truth));
                });

        _.test("toolong", []() {
                polysync::node truth("array", std::vector<std::uint8_t> { 1, 2, 3 });
                polysync::node toolong("array", std::vector<std::uint8_t> { 1, 2, 3, 4 });
                expect(toolong, not_equal_to(truth));
                });
});

mettle::suite<> tree("tree", [](auto& _) {

        _.test("equal", []() {
                polysync::tree equal = polysync::tree("type", {
                                { "start_time", std::uint16_t { 1 } },
                                { "scanner_count", std::uint32_t { 2 } }
                                });

                polysync::tree truth = polysync::tree("type", {
                                { "start_time", std::uint16_t { 1 } },
                                { "scanner_count", std::uint32_t { 2 } }
                                });

                expect(equal, equal_to(truth));
                });

        _.test("notequal", []() {
                polysync::tree notequal = polysync::tree("type", {
                                { "start_time", std::uint16_t { 1 } },
                                { "scanner_count", std::uint32_t { 3 } }
                        });

                polysync::tree truth = polysync::tree("type", {
                                { "start_time", std::uint16_t { 1 } },
                                { "scanner_count", std::uint32_t { 2 } }
                        });

                expect(notequal, not_equal_to(truth));
                });

        _.test("tooshort", []() {
                polysync::tree tooshort = polysync::tree("type", {
                                { "start_time", std::uint16_t { 1 } },
                        });

                polysync::tree truth = polysync::tree("type", {
                                { "start_time", std::uint16_t { 1 } },
                                { "scanner_count", std::uint32_t { 2 } }
                        });

                expect(tooshort, not_equal_to(truth));
                });

        _.test("toolong", []() {
                polysync::tree toolong = polysync::tree("type", {
                                { "start_time", std::uint16_t { 1 } },
                                { "scanner_count", std::uint32_t { 2 } },
                                { "stop_time", std::uint16_t { 3 } },
                        });

                polysync::tree truth = polysync::tree("type", {
                                { "start_time", std::uint16_t { 1 } },
                                { "scanner_count", std::uint32_t { 2 } }
                        });

                expect(toolong, not_equal_to(truth));
                });
});

mettle::suite<> nested_tree("nested_tree", [](auto& _) {

        _.test("equal", []() {
                polysync::tree equal = polysync::tree("type", {
                        { "header", std::uint16_t { 1 } },
                                { "start_time", std::uint16_t { 2 } },
                                { "scanner_count", std::uint32_t { 3 } }
                        });

                polysync::tree truth = polysync::tree("type", {
                        { "header", std::uint16_t { 1 } },
                                { "start_time", std::uint16_t { 2 } },
                                { "scanner_count", std::uint32_t { 3 } }
                        });

                expect(equal, equal_to(truth));
                });

        _.test("notequal", []() {
                polysync::tree notequal = polysync::tree("type", {
                        { "header", std::uint16_t { 1 } },
                                { "start_time", std::uint16_t { 2 } },
                                { "scanner_count", std::uint32_t { 4 } }
                        });

                polysync::tree truth = polysync::tree("type", {
                        { "header", std::uint16_t { 1 } },
                                { "start_time", std::uint16_t { 2 } },
                                { "scanner_count", std::uint32_t { 3 } }
                        });

                expect(notequal, not_equal_to(truth));
                });

        _.test("tooshort", []() {
                polysync::tree tooshort = polysync::tree("type", {
                        { "header", std::uint16_t { 1 } },
                                { "start_time", std::uint16_t { 2 } },
                        });

                polysync::tree truth = polysync::tree("type", {
                        { "header", std::uint16_t { 1 } },
                                { "start_time", std::uint16_t { 2 } },
                                { "scanner_count", std::uint32_t { 3 } }
                        });

                expect(tooshort, not_equal_to(truth));
                });

        _.test("toolong", []() {
                polysync::tree toolong = polysync::tree("type", {
                        { "header", std::uint16_t { 1 } },
                                { "start_time", std::uint16_t { 2 } },
                                { "scanner_count", std::uint32_t { 3 } },
                                { "stop_time", std::uint16_t { 4 } },
                        });

                polysync::tree truth = polysync::tree("type", {
                        { "header", std::uint16_t { 1 } },
                                { "start_time", std::uint16_t { 2 } },
                                { "scanner_count", std::uint32_t { 3 } }
                        });

                expect(toolong, not_equal_to(truth));
                });
});


