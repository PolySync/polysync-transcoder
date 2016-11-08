#include <mettle.hpp>

#include <polysync/exception.hpp>
#include <polysync/plog/decoder.hpp>
#include <polysync/plog/io.hpp>
#include <polysync/logging.hpp>
#include <polysync/console.hpp>
#include "types.hpp"

using namespace mettle;
namespace plog = polysync::plog;
namespace mp = boost::multiprecision;

plog::tree decode_hex(const plog::descriptor::type& desc, const std::string& hex) {
    // Form a binary string from a hex string
    std::string blob;
    mp::export_bits(mp::cpp_int("0x" + hex), std::back_inserter(blob), 8);

    // decode a plog::tree from the description
    std::stringstream stream(blob);
    plog::decoder decode(stream);
    plog::tree result = *decode(desc).target<plog::tree>();

    if (stream.tellg() < decode.endpos)
        result->emplace_back(decode.decode_desc("raw"));
    return result;
}

mettle::suite<> decode("plog::decode", [](auto& _) {
        polysync::console::format = polysync::console::nocolor();

        _.teardown([]() {
                plog::descriptor::catalog.clear();
            });
        
        _.test("simple", []() {
                std::string hex = "0100000000000000" "02000000" "03000000";
                plog::tree truth = plog::tree::create({
                        { "dest_guid", plog::guid { 1 } },
                        { "data_type", std::uint32_t { 2 } },
                        { "payload", std::uint32_t { 3 } }
                    });

                plog::tree tree = decode_hex(descriptor::ps_byte_array_msg, hex);
                expect(tree, equal_to(truth));
                });

        _.test("skip", []() {
                std::string hex = 
                    "01000000" "02000000" "03" 
                    "00000000" // 4 byte skip
                    "04" "0500" "0600000000000000";

                plog::tree truth = plog::tree::create({
                        { "magic", std::uint32_t { 1 } },
                        { "prev_size", std::uint32_t { 2 } },
                        { "size", std::uint8_t { 3 } },
                        { "device_id", std::uint8_t { 4 } },
                        { "data_type", std::uint16_t { 5 } },
                        { "time", std::uint64_t { 6 } },
                    });

                plog::tree tree = decode_hex(descriptor::ibeo_header, hex);
                expect(tree, equal_to(truth));
                });

        _.test("unknown_type", []() {
                plog::descriptor::type desc { "incomplete_type", { 
                    { "dest_guid", typeid(plog::guid) },
                    { "data_type", typeid(std::uint32_t) },
                    { "time", typeid(void) } }
                    };

                std::string hex = "0100000000000000" "02000000" "0300000000000000";
                // This line tickles a gdb bug.
                // expect([=]() { decode_hex(desc, hex); }, 
                //         thrown<polysync::error>("no typemap for \"time\""));

                });

        _.subsuite("nested_type", [](auto&_) {

                plog::descriptor::type scanner_info { "scanner_info", {
                    { "device_id", typeid(std::uint8_t) },
                    { "scanner_type", typeid(std::uint8_t) } }
                };
                plog::descriptor::catalog.emplace("scanner_info", scanner_info);
                plog::descriptor::type container { "container", {
                    { "start_time", typeid(std::uint16_t) },
                    { "scanner_info_list", plog::descriptor::nested{"scanner_info"} } }
                };
                
                plog::tree truth = plog::tree::create({
                        { "start_time", std::uint16_t { 1 } },
                        { "scanner_info", plog::tree::create({ 
                                { "device_id", std::uint8_t { 2 } }, 
                                { "scanner_type", std::uint8_t { 3 } }
                                }) }
                    });

                _.test("equal", [=]() {
                        plog::tree correct = decode_hex(container, "0100" "02" "03");
                        expect(correct, equal_to(truth));
                        });

                _.test("notequal", [=]() {
                        plog::tree wrong = decode_hex(container, "0100" "02" "04");
                        expect(wrong, not_equal_to(truth));
                        });

                _.test("tooshort", [=]() {
                        plog::tree tooshort = decode_hex(container, "0100" "02");
                        expect(tooshort, not_equal_to(truth));
                        });

                _.test("toolong", [=]() {
                        plog::tree toolong = decode_hex(container, "0100" "02" "03" "04");
                        expect(toolong, not_equal_to(truth));
                        });
            });

        _.subsuite("terminal_array", [](auto &_) {

                plog::descriptor::type sometype { "sometype", {
                    { "time", typeid(std::uint16_t) },
                    { "data", plog::descriptor::terminal_array { 3, typeid(std::uint8_t) } }
                } };

                plog::tree truth = plog::tree::create({
                        { "time", std::uint16_t { 1 } },
                        { "data", std::vector<uint8_t> { 2, 3, 4 } }
                    });

                _.test("equal", [=]() {
                        plog::tree correct = decode_hex(sometype, "0100" "02" "03" "04");
                        expect(correct, equal_to(truth));
                        });

                _.test("notequal", [=]() {
                        plog::tree wrong = decode_hex(sometype, "0100" "02" "04" "04");
                        expect(wrong, not_equal_to(truth));
                        });

                _.test("tooshort", [=]() {
                        plog::tree tooshort = decode_hex(sometype, "0100" "02" "03");
                        expect(tooshort, not_equal_to(truth));
                        });

                _.test("toolong", [=]() {
                        plog::tree toolong = decode_hex(sometype, "0100" "02" "03" "04" "05");
                        expect(toolong, not_equal_to(truth));
                        });

            });

        _.subsuite("nested_array", [](auto &_) {
                _.test("equal", [=]() {
                plog::descriptor::type scanner_info { "scanner_info", {
                    { "device_id", typeid(std::uint8_t) },
                    { "scanner_type", typeid(std::uint8_t) } }
                };
                plog::descriptor::catalog.emplace("scanner_info", scanner_info);
                plog::descriptor::type container { "container", {
                    { "start_time", typeid(std::uint16_t) },
                    { "scanner_info_list", plog::descriptor::nested_array{ 3, scanner_info } } }
                };

                plog::tree truth = plog::tree::create({
                        { "time", std::uint16_t { 1 } },
                        { "scanner_info", std::vector<plog::tree> {
                                plog::tree::create({ 
                                        { "device_id", std::uint8_t { 2 } },
                                        { "scanner_type", std::uint8_t { 3 } },
                                        }),
                                plog::tree::create({ 
                                        { "device_id", std::uint8_t { 4 } },
                                        { "scanner_type", std::uint8_t { 5 } }, 
                                        }),
                                plog::tree::create({ 
                                        { "device_id", std::uint8_t { 6 } },
                                        { "scanner_type", std::uint8_t { 7 } } 
                                        }),
                        }}
                    });

                plog::tree correct = decode_hex(container, 
                        "0100" 
                        "02" "03" 
                        "04" "05"
                        "06" "07"
                        );
                expect(correct, equal_to(truth));
                });
        });
});

