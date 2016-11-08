#include <mettle.hpp>

#include <polysync/plog/encoder.hpp>
#include <polysync/console.hpp>
#include "types.hpp"

using namespace mettle;
namespace plog = polysync::plog;
namespace mp = boost::multiprecision;

std::string encode_tree(plog::tree tree, const plog::descriptor::type& desc) {
    std::stringstream stream;
    plog::encoder encode(stream);
    encode(tree, desc);

    mp::cpp_int blob;
    std::string bin = stream.str();
    mp::import_bits(blob, bin.begin(), bin.end(), 8);

    std::stringstream hex;
    hex << std::setfill('0') << std::setw(2*bin.size()) << std::hex << blob;
    return hex.str();
}

mettle::suite<> encode("plog::encode", [](auto& _) {

        _.test("simple", []() {
                std::string truth = "0100000000000000" "02000000" "03000000";
                plog::tree simple = plog::tree::create({
                        { "dest_guid", plog::guid { 1 } },
                        { "data_type", std::uint32_t { 2 } },
                        { "payload", std::uint32_t { 3 } }
                        });

                std::string enc = encode_tree(simple, descriptor::ps_byte_array_msg);
                expect(enc, equal_to(truth));
                });

        _.test("skip", []() {
                std::string truth = 
                "01000000" "02000000" "03" 
                "00000000" // 4 byte skip
                "04" "0500" "0600000000000000";

                plog::tree header = plog::tree::create({
                        { "magic", std::uint32_t { 1 } },
                        { "prev_size", std::uint32_t { 2 } },
                        { "size", std::uint8_t { 3 } },
                        { "device_id", std::uint8_t { 4 } },
                        { "data_type", std::uint16_t { 5 } },
                        { "time", std::uint64_t { 6 } },
                        });

                std::string enc = encode_tree(header, descriptor::ibeo_header);
                expect(enc, equal_to(truth));
                });

        _.test("unknown_type", []() {

                plog::descriptor::type desc { "incomplete_type", { 
                    { "dest_guid", typeid(plog::guid) },
                    { "data_type", typeid(std::uint32_t) },
                    { "payload", typeid(void) } }
                };

                plog::tree simple = plog::tree::create({
                        { "dest_guid", plog::guid { 1 } },
                        { "data_type", std::uint32_t { 2 } },
                        { "payload", std::uint32_t { 3 } }
                        });

                // This line tickles a gdb bug.
                expect([=]() { encode_tree(simple, desc); }, 
                        thrown<polysync::error>("unknown type for field \"payload\""));
                });

        _.subsuite("nested_type", [](auto&_) {

                plog::descriptor::type scanner_info { "scanner_info", {
                    { "device_id", typeid(std::uint8_t) },
                    { "scanner_type", typeid(std::uint8_t) } }
                };
                plog::descriptor::catalog.emplace("scanner_info", scanner_info);

                plog::descriptor::type container { "container", {
                    { "start_time", typeid(std::uint16_t) },
                    { "scanner_info_list", plog::descriptor::nested { "scanner_info" } } }
                };

                std::string truth = "0100" "02" "03";

                _.test("equal", [=]() {
                        plog::tree nest = plog::tree::create({
                                { "start_time", std::uint16_t { 1 } },
                                { "scanner_info_list", plog::tree::create({ 
                                        { "device_id", std::uint8_t { 2 } }, 
                                        { "scanner_type", std::uint8_t { 3 } }
                                        }) }
                                });

                        std::string enc = encode_tree(nest, container);
                        expect(enc, equal_to(truth));
                        });

                _.test("not_equal", [=]() {
                        plog::tree nest = plog::tree::create({
                                { "start_time", std::uint16_t { 1 } },
                                { "scanner_info_list", plog::tree::create({ 
                                        { "device_id", std::uint8_t { 2 } }, 
                                        { "scanner_type", std::uint8_t { 4 } }
                                        }) }
                                });

                        std::string enc = encode_tree(nest, container);
                        expect(enc, not_equal_to(truth));
                        });

                _.test("too_short", [=]() {
                        plog::tree nest = plog::tree::create({
                                { "start_time", std::uint16_t { 1 } },
                                { "scanner_info_list", plog::tree::create({ 
                                        { "device_id", std::uint8_t { 2 } }, 
                                        }) }
                                });

                        // Throw on missing fields.  In the future, we may have a mask instead.
                        expect([=]() { encode_tree(nest, container); }, 
                                thrown<polysync::error>("field \"scanner_type\" not found in tree"));
                        });

                _.test("extra_data", [=]() {
                        plog::tree nest = plog::tree::create({
                                { "start_time", std::uint16_t { 1 } },
                                { "scanner_info_list", plog::tree::create({ 
                                        { "device_id", std::uint8_t { 2 } }, 
                                        { "scanner_type", std::uint8_t { 3 } },
                                        { "more_data", std::uint8_t { 4 } }
                                        }) }
                                });

                        std::string enc = encode_tree(nest, container);
                        // Silently ignore extra data.
                        expect(enc, equal_to(truth));
                        });
        });

        _.subsuite("terminal_array", [](auto &_) {

                plog::descriptor::type sometype { "sometype", {
                    { "time", typeid(std::uint16_t) },
                    { "data", plog::descriptor::terminal_array { 3, typeid(std::uint8_t) } }
                } };

                std::string truth = "0100" "02" "03" "04";

                _.test("equal", [=]() {
                        plog::tree tree = plog::tree::create({
                                { "time", std::uint16_t { 1 } },
                                { "data", std::vector<std::uint8_t> { 2, 3, 4 } }
                                });
                        std::string enc = encode_tree(tree, sometype);
                        expect(enc, equal_to(truth));
                        });

                _.test("not_equal", [=]() {
                        plog::tree tree = plog::tree::create({
                                { "time", std::uint16_t { 1 } },
                                { "data", std::vector<std::uint8_t> { 2, 5, 4 } }
                                });
                        std::string enc = encode_tree(tree, sometype);
                        expect(enc, not_equal_to(truth));
                        });

                _.test("type_error", [=]() {
                        plog::tree tree = plog::tree::create({
                                { "time", std::uint16_t { 1 } },
                                { "data", std::vector<std::uint16_t> { 2, 5, 4 } }
                                });
                        expect([=]() { encode_tree(tree, sometype); },
                                thrown<polysync::error>("mismatched type in field \"data\""));
                        });

                _.test("too_short", [=]() {
                        plog::tree tree = plog::tree::create({
                                { "time", std::uint16_t { 1 } },
                                { "data", std::vector<std::uint8_t> { 2, 3 } }
                                });
                        expect([=]() { encode_tree(tree, sometype); },
                                thrown<polysync::error>("mismatched size in field \"data\""));
                        });

                _.test("too_long", [=]() {
                        plog::tree tree = plog::tree::create({
                                { "time", std::uint16_t { 1 } },
                                { "data", std::vector<uint8_t> { 2, 3, 4, 5 } }
                                });
                        expect([=]() { encode_tree(tree, sometype); },
                                thrown<polysync::error>("mismatched size in field \"data\""));
                        });

        });

        // Define a desciption fixture for the nested_array subsuite.
        struct scanner_info : plog::descriptor::type {
            scanner_info() : plog::descriptor::type { "scanner_info", {
                { "device_id", typeid(std::uint8_t) },
                { "scanner_type", typeid(std::uint8_t) } } } 
            {
                plog::descriptor::catalog.emplace("scanner_info", *this);
            }
        };

        struct container : plog::descriptor::type {
            scanner_info info;
            container() : plog::descriptor::type { "container", {
                { "start_time", typeid(std::uint16_t) },
                { "scanner_info_list", plog::descriptor::nested_array { 2, info } } } } 
            {
            }
        };

        mettle::subsuite<container>(_, "nested_array", [](auto &_) {

                std::string truth = "0100" "02" "03" "04" "05";

                _.test("equal", [=](const container& c) {
                        plog::tree tree = plog::tree::create({
                                { "start_time", std::uint16_t { 1 } },
                                { "scanner_info_list", std::vector<plog::tree> {
                                        plog::tree::create({ 
                                                { "device_id", std::uint8_t { 2 } },
                                                { "scanner_type", std::uint8_t { 3 } },
                                                }),
                                        plog::tree::create({ 
                                                { "device_id", std::uint8_t { 4 } },
                                                { "scanner_type", std::uint8_t { 5 } }, 
                                                }),
                                        }}
                                });
                        std::string enc = encode_tree(tree, c);
                        expect(enc, equal_to(truth));
                        });

                _.test("not_equal", [=](const container& c) {
                        plog::tree tree = plog::tree::create({
                                { "start_time", std::uint16_t { 1 } },
                                { "scanner_info_list", std::vector<plog::tree> {
                                        plog::tree::create({ 
                                                { "device_id", std::uint8_t { 2 } },
                                                { "scanner_type", std::uint8_t { 3 } },
                                                }),
                                        plog::tree::create({ 
                                                { "device_id", std::uint8_t { 4 } },
                                                { "scanner_type", std::uint8_t { 6 } }, 
                                                }),
                                        }}});
                        std::string enc = encode_tree(tree, c);
                        expect(enc, not_equal_to(truth));
                        });

                _.test("missing_array", [=](const container& c) {
                        plog::tree tree = plog::tree::create({
                                { "start_time", std::uint16_t { 1 } },
                                });
                        expect([=]() { encode_tree(tree, c); },
                                thrown<polysync::error>("field \"scanner_info_list\" not found in tree"));
                        });

        });

        _.subsuite("order", [](auto&_) {
                _.test("same", [=]() {

                        plog::tree tree = plog::tree::create({
                                { "dest_guid", plog::guid { 1 } },
                                { "data_type", std::uint32_t { 2 } },
                                { "payload", std::uint32_t { 3 } }
                                });

                        std::stringstream stream;
                        plog::encoder encode(stream);
                        encode(tree, descriptor::ps_byte_array_msg);

                        expect(stream.str(), bits("0100000000000000" "02000000" "03000000"));
                        });

                _.test("swapped", [=]() {
                        plog::tree tree = plog::tree::create({
                                { "data_type", std::uint32_t { 2 } },
                                { "payload", std::uint32_t { 3 }  },
                                { "dest_guid", plog::guid { 1 } },
                                });

                        std::stringstream stream;
                        plog::encoder encode(stream);
                        encode(tree, descriptor::ps_byte_array_msg);

                        expect(stream.str(), bits("0100000000000000" "02000000" "03000000"));
                        });

                _.test("skip-same", [=]() {

                        plog::tree tree = plog::tree::create({
                                { "magic", std::uint32_t { 1 }, },
                                { "prev_size", std::uint32_t { 2 } },
                                { "size", std::uint8_t { 3 } },
                                { "device_id", std::uint8_t { 4 } },
                                { "data_type", std::uint16_t { 5 } },
                                { "time", std::uint64_t { 6 } },
                                });

                        std::stringstream stream;
                        plog::encoder encode(stream);
                        encode(tree, descriptor::ibeo_header);

                        expect(stream.str(), 
                                bits("01000000" "02000000" "03" 
                                    "00000000" // 4 byte skip
                                    "04" "0500" "0600000000000000"));
                        });

                _.test("skip-swapped", [=]() {

                        plog::tree tree = plog::tree::create({
                                { "magic", std::uint32_t { 1 } },
                                { "data_type", std::uint16_t { 5 } },
                                { "prev_size", std::uint32_t { 2 } },
                                { "size", std::uint8_t { 3 } },
                                { "device_id", std::uint8_t { 4 } },
                                { "time", std::uint64_t { 6 } },
                                });

                        std::stringstream stream;
                        plog::encoder encode(stream);
                        encode(tree, descriptor::ibeo_header);

                        expect(stream.str(), 
                                bits("01000000" "02000000" "03" 
                                    "00000000" // 4 byte skip
                                    "04" "0500" "0600000000000000"));
                        });
        });

});


