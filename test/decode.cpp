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
    return *decode(desc).target<plog::tree>();
}

mettle::suite<> decode("plog::decode", [](auto& _) {

        _.teardown([]() {
                plog::descriptor::catalog.clear();
            });


        
        _.test("simple", []() {
                std::string hex = "0100000000000000" "02000000" "03000000";
                plog::tree truth = plog::tree::create({
                        { plog::guid { 1 }, "dest_guid" },
                        { std::uint32_t { 2 }, "data_type" },
                        { std::uint32_t { 3 }, "payload" }
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
                        { std::uint32_t { 1 }, "magic" },
                        { std::uint32_t { 2 }, "prev_size" },
                        { std::uint8_t { 3 }, "size" },
                        { std::uint8_t { 4 }, "device_id" },
                        { std::uint16_t { 5 }, "data_type" },
                        { std::uint64_t { 6 }, "time" },
                    });

                plog::tree tree = decode_hex(descriptor::ibeo_header, hex);
                expect(tree, equal_to(truth));
                });

        _.test("unknown_type", []() {
                plog::descriptor::type desc { "incomplete_type", { 
                    { "dest_guid", "ps_guid" },
                    { "data_type", "uint32" },
                    { "time", "NTP64" } }
                    };

                std::string hex = "0100000000000000" "02000000" "0300000000000000";
                expect([=]() { decode_hex(desc, hex); }, 
                        thrown<polysync::error>("no decoder for type NTP64"));

                });

        _.test("nested_type", []() {
                plog::descriptor::type scanner_info { "scanner_info", {
                    { "device_id", "uint8" },
                    { "scanner_type", "uint8" } }
                };
                plog::descriptor::catalog.emplace("scanner_info", scanner_info);
                plog::descriptor::type desc { "container", {
                    { "start_time", "uint16" },
                    { "scanner_info_list", "scanner_info" } }
                };
                
                std::string hex = "0100" "02" "03";
                plog::tree tree = decode_hex(desc, hex);

                plog::tree truth = plog::tree::create({
                        { std::uint16_t { 1 }, "start_time" },
                        { plog::tree::create({ 
                                { std::uint8_t { 2 }, "device_id" }, 
                                { std::uint8_t { 3 }, "scanner_type" }
                                }), "scanner_info" }
                    });

                expect(tree, equal_to(truth));
            });

        _.test("fixed_array", []() {
                plog::descriptor::type desc { "sometype", {
                    { "time", "uint16" },
                    { "data", descriptor::sequence(2) }
                } }
            });
        });

