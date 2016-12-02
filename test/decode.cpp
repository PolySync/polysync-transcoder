#include <mettle.hpp>

#include <polysync/exception.hpp>
#include <polysync/plog/decoder.hpp>
#include <polysync/print_hana.hpp>
#include <polysync/logging.hpp>
#include <polysync/console.hpp>
#include "types.hpp"

using namespace mettle;
namespace plog = polysync::plog;
namespace mp = boost::multiprecision;

polysync::tree decode_hex(const polysync::descriptor::type& desc, const std::string& hex) {
    // Form a binary string from a hex string
    std::string blob;
    mp::export_bits(mp::cpp_int("0x" + hex), std::back_inserter(blob), 8);

    // decode a polysync::tree from the description
    std::stringstream stream(blob);
    plog::decoder decode(stream);
    polysync::tree result = *decode(desc).target<polysync::tree>();

    if (stream.tellg() < decode.endpos)
        result->emplace_back("bytes", decode.decode_desc("raw"));
    return result;
}

mettle::suite<> decode("plog::decode", [](auto& _) {

        _.teardown([]() {
                polysync::descriptor::catalog.clear();
            });
        
        _.test("simple", []() {
                std::string hex = "0100000000000000" "02000000" "03000000";
                polysync::tree truth = polysync::tree("type", {
                        { "dest_guid", plog::guid { 1 } },
                        { "data_type", std::uint32_t { 2 } },
                        { "payload", std::uint32_t { 3 } }
                    });

                polysync::tree tree = decode_hex(descriptor::ps_byte_array_msg, hex);
                expect(tree, equal_to(truth));
                });

        _.test("skip", []() {
                std::string hex = 
                    "01000000" "02000000" "03" 
                    "DEADBEEF" // 4 byte skip
                    "04" "0500" "0600000000000000";

                polysync::tree truth = polysync::tree("type", {
                        { "magic", std::uint32_t { 1 } },
                        { "prev_size", std::uint32_t { 2 } },
                        { "size", std::uint8_t { 3 } },
                        { "skip-1", polysync::bytes { 0xDE, 0xAD, 0xBE, 0xEF } },
                        { "device_id", std::uint8_t { 4 } },
                        { "data_type", std::uint16_t { 5 } },
                        { "time", std::uint64_t { 6 } },
                    });

                polysync::tree tree = decode_hex(descriptor::ibeo_header, hex);
                expect(tree, equal_to(truth));
                });

        _.test("unknown_type", []() {
                polysync::descriptor::type desc { "incomplete_type", { 
                    { "dest_guid", typeid(plog::guid) },
                    { "data_type", typeid(std::uint32_t) },
                    { "time", typeid(void) } }
                    };

                std::string hex = "0100000000000000" "02000000" "0300000000000000";
                // This line tickles a gdb bug.
                expect([=]() { decode_hex(desc, hex); }, 
                        thrown<polysync::error>("no typemap"));

                });

        _.subsuite("nested_type", [](auto&_) {

                polysync::descriptor::type scanner_info { "scanner_info", {
                    { "device_id", typeid(std::uint8_t) },
                    { "scanner_type", typeid(std::uint8_t) } }
                };
                polysync::descriptor::catalog.emplace("scanner_info", scanner_info);
                polysync::descriptor::type container { "container", {
                    { "start_time", typeid(std::uint16_t) },
                    { "scanner_info_list", polysync::descriptor::nested{"scanner_info"} } }
                };
                
                polysync::tree truth = polysync::tree("type", {
                        { "start_time", std::uint16_t { 1 } },
                        { "scanner_info", polysync::tree("type", { 
                                { "device_id", std::uint8_t { 2 } }, 
                                { "scanner_type", std::uint8_t { 3 } }
                                }) }
                    });

                _.test("equal", [=]() {
                        polysync::tree correct = decode_hex(container, "0100" "02" "03");
                        expect(correct, equal_to(truth));
                        });

                _.test("notequal", [=]() {
                        polysync::tree wrong = decode_hex(container, "0100" "02" "04");
                        expect(wrong, not_equal_to(truth));
                        });

                _.test("tooshort", [=]() {
                        polysync::tree tooshort = decode_hex(container, "0100" "02");
                        expect(tooshort, not_equal_to(truth));
                        });

                _.test("toolong", [=]() {
                        polysync::tree toolong = decode_hex(container, "0100" "02" "03" "04");
                        expect(toolong, not_equal_to(truth));
                        });
            });

        _.subsuite("terminal_array", [](auto &_) {

                polysync::descriptor::type sometype { "sometype", {
                    { "time", typeid(std::uint16_t) },
                    { "data", polysync::descriptor::array { 3, typeid(std::uint8_t) } }
                } };

                polysync::tree truth = polysync::tree("type", {
                        { "time", std::uint16_t { 1 } },
                        { "data", std::vector<uint8_t> { 2, 3, 4 } }
                    });

                _.test("equal", [=]() {
                        polysync::tree correct = decode_hex(sometype, "0100" "02" "03" "04");
                        expect(correct, equal_to(truth));
                        });

                _.test("notequal", [=]() {
                        polysync::tree wrong = decode_hex(sometype, "0100" "02" "04" "04");
                        expect(wrong, not_equal_to(truth));
                        });

                _.test("tooshort", [=]() {
                        polysync::tree tooshort = decode_hex(sometype, "0100" "02" "03");
                        expect(tooshort, not_equal_to(truth));
                        });

                _.test("toolong", [=]() {
                        polysync::tree toolong = decode_hex(sometype, "0100" "02" "03" "04" "05");
                        expect(toolong, not_equal_to(truth));
                        });

            });

        _.subsuite("nested_array", [](auto &_) {
                _.test("equal", [=]() {
                polysync::descriptor::type scanner_info { "scanner_info", {
                    { "device_id", typeid(std::uint8_t) },
                    { "scanner_type", typeid(std::uint8_t) } }
                };
                polysync::descriptor::catalog.emplace("scanner_info", scanner_info);
                polysync::descriptor::type container { "container", {
                    { "start_time", typeid(std::uint16_t) },
                    { "scanner_info_list", polysync::descriptor::array{ 3, "scanner_info" } } }
                };

                polysync::tree truth = polysync::tree("type", {
                        { "time", std::uint16_t { 1 } },
                        { "scanner_info", std::vector<polysync::tree> {
                                polysync::tree("type", { 
                                        { "device_id", std::uint8_t { 2 } },
                                        { "scanner_type", std::uint8_t { 3 } },
                                        }),
                                polysync::tree("type", { 
                                        { "device_id", std::uint8_t { 4 } },
                                        { "scanner_type", std::uint8_t { 5 } }, 
                                        }),
                                polysync::tree("type", { 
                                        { "device_id", std::uint8_t { 6 } },
                                        { "scanner_type", std::uint8_t { 7 } } 
                                        }),
                        }}
                    });

                polysync::tree correct = decode_hex(container, 
                        "0100" 
                        "02" "03" 
                        "04" "05"
                        "06" "07"
                        );
                expect(correct, equal_to(truth));
                });
        });

        _.subsuite("dynamic_array", [](auto &_) {

                polysync::descriptor::type sometype { "dynarray", {
                    { "points", typeid(std::uint16_t) },
                    { "data", polysync::descriptor::array { "points", typeid(std::uint8_t) } }
                } };

                polysync::tree truth = polysync::tree("type", {
                        { "points", std::uint16_t { 3 } },
                        { "data", std::vector<uint8_t> { 2, 3, 4 } }
                    });

                _.test("equal", [=]() {
                        polysync::tree correct = decode_hex(sometype, "0300" "02" "03" "04");
                        expect(correct, equal_to(truth));
                        });

            });

});

