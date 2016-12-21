#include <mettle.hpp>

#include <polysync/plog/encoder.hpp>
#include <polysync/console.hpp>
#include "types.hpp"

using namespace mettle;
namespace plog = polysync::plog;
namespace mp = boost::multiprecision;

std::string encode_tree(polysync::tree tree, const polysync::descriptor::Type& desc) {
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
                polysync::tree simple = polysync::tree("type", {
                        { "dest_guid", plog::ps_guid { 1 } },
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

                polysync::tree header = polysync::tree("type", {
                        { "magic", std::uint32_t { 1 } },
                        { "prev_size", std::uint32_t { 2 } },
                        { "size", std::uint8_t { 3 } },
                        { "skip-1", polysync::bytes { 0, 0, 0, 0 } },
                        { "device_id", std::uint8_t { 4 } },
                        { "data_type", std::uint16_t { 5 } },
                        { "time", std::uint64_t { 6 } },
                        });

                std::string enc = encode_tree(header, descriptor::ibeo_header);
                expect(enc, equal_to(truth));
                });

        _.test("unknown_type", []() {

                polysync::descriptor::Type desc { "incomplete_type", {
                    { "dest_guid", typeid(plog::ps_guid) },
                    { "data_type", typeid(std::uint32_t) },
                    { "payload", typeid(void) } } // void is the unknown type
                };

                polysync::tree simple = polysync::tree("type", {
                        { "dest_guid", plog::ps_guid { 1 } },
                        { "data_type", std::uint32_t { 2 } },
                        { "payload", std::uint32_t { 3 } }
                        });

                // This line tickles a gdb bug.
                expect([=]() { encode_tree(simple, desc); },
                        thrown<polysync::error>("no typemap"));
                });

        _.subsuite("nested_type", [](auto&_) {

                polysync::descriptor::Type scanner_info { "scanner_info", {
                    { "device_id", typeid(std::uint8_t) },
                    { "scanner_type", typeid(std::uint8_t) } }
                };
                polysync::descriptor::catalog.emplace("scanner_info", scanner_info);

                polysync::descriptor::Type container { "container", {
                    { "start_time", typeid(std::uint16_t) },
                    { "scanner_info_list", polysync::descriptor::Nested { "scanner_info" } } }
                };

                std::string truth = "0100" "02" "03";

                _.test("equal", [=]() {
                        polysync::tree nest = polysync::tree("type", {
                                { "start_time", std::uint16_t { 1 } },
                                { "scanner_info_list", polysync::tree("type", {
                                        { "device_id", std::uint8_t { 2 } },
                                        { "scanner_type", std::uint8_t { 3 } }
                                        }) }
                                });

                        std::string enc = encode_tree(nest, container);
                        expect(enc, equal_to(truth));
                        });

                _.test("not_equal", [=]() {
                        polysync::tree nest = polysync::tree("type", {
                                { "start_time", std::uint16_t { 1 } },
                                { "scanner_info_list", polysync::tree("type", {
                                        { "device_id", std::uint8_t { 2 } },
                                        { "scanner_type", std::uint8_t { 4 } }
                                        }) }
                                });

                        std::string enc = encode_tree(nest, container);
                        expect(enc, not_equal_to(truth));
                        });

                _.test("too_short", [=]() {
                        polysync::tree nest = polysync::tree("type", {
                                { "start_time", std::uint16_t { 1 } },
                                { "scanner_info_list", polysync::tree("type", {
                                        { "device_id", std::uint8_t { 2 } },
                                        }) }
                                });

                        // Throw on missing fields.  In the future, we may have a mask instead.
                        expect([=]() { encode_tree(nest, container); },
                                thrown<polysync::error>("field \"scanner_type\" not found in tree"));
                        });

                _.test("extra_data", [=]() {
                        polysync::tree nest = polysync::tree("type", {
                                { "start_time", std::uint16_t { 1 } },
                                { "scanner_info_list", polysync::tree("type", {
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

                polysync::descriptor::Type sometype { "sometype", {
                    { "time", typeid(std::uint16_t) },
                    { "data", polysync::descriptor::Array { 3, typeid(std::uint8_t) } }
                } };

                std::string truth = "0100" "02" "03" "04";

                _.test("equal", [=]() {
                        polysync::tree tree = polysync::tree("type", {
                                { "time", std::uint16_t { 1 } },
                                { "data", std::vector<std::uint8_t> { 2, 3, 4 } }
                                });
                        std::string enc = encode_tree(tree, sometype);
                        expect(enc, equal_to(truth));
                        });

                _.test("not_equal", [=]() {
                        polysync::tree tree = polysync::tree("type", {
                                { "time", std::uint16_t { 1 } },
                                { "data", std::vector<std::uint8_t> { 2, 5, 4 } }
                                });
                        std::string enc = encode_tree(tree, sometype);
                        expect(enc, not_equal_to(truth));
                        });

                _.test("type_error", [=]() {
                        polysync::tree tree = polysync::tree("type", {
                                { "time", std::uint16_t { 1 } },
                                { "data", std::vector<std::uint16_t> { 2, 5, 4 } }
                                });
                        expect([=]() { encode_tree(tree, sometype); },
                                thrown<polysync::error>("mismatched type in field \"data\""));
                        });

                _.test("too_short", [=]() {
                        polysync::tree tree = polysync::tree("type", {
                                { "time", std::uint16_t { 1 } },
                                { "data", std::vector<std::uint8_t> { 2, 3 } }
                                });
                        expect([=]() { encode_tree(tree, sometype); },
                                thrown<polysync::error>("mismatched size in field \"data\""));
                        });

                _.test("too_long", [=]() {
                        polysync::tree tree = polysync::tree("type", {
                                { "time", std::uint16_t { 1 } },
                                { "data", std::vector<uint8_t> { 2, 3, 4, 5 } }
                                });
                        expect([=]() { encode_tree(tree, sometype); },
                                thrown<polysync::error>("mismatched size in field \"data\""));
                        });

        });

        // Define a desciption fixture for the nested_array subsuite.
        struct scanner_info : polysync::descriptor::Type {
            scanner_info() : polysync::descriptor::Type { "scanner_info", {
                { "device_id", typeid(std::uint8_t) },
                { "scanner_type", typeid(std::uint8_t) } } }
            {
                polysync::descriptor::catalog.emplace("scanner_info", *this);
            }
        };

        struct container : polysync::descriptor::Type {
            container() : polysync::descriptor::Type { "container", {
                { "start_time", typeid(std::uint16_t) },
                { "scanner_info_list", polysync::descriptor::Array { 2, "scanner_info" } } } }
            {
            }
        };

        mettle::subsuite<container>(_, "dynamic_array", [](auto &_) {

                std::string truth = "0300" "02" "03" "04";

                _.test("equal", [=](const container& c) {
                        polysync::descriptor::Type intvec { "intvec", {
                            { "points", typeid(std::uint16_t) },
                            { "data", polysync::descriptor::Array { "points", typeid(std::uint8_t) } } }
                        };

                        polysync::tree tree = polysync::tree("type", {
                                { "points", std::uint16_t { 3 } },
                                { "data", std::vector<uint8_t> { 2, 3, 4 } }
                                });
                        std::string enc = encode_tree(tree, intvec);
                        expect(enc, equal_to(truth));
                        });

        });

        mettle::subsuite<container>(_, "nested_array", [](auto &_) {

                std::string truth = "0100" "02" "03" "04" "05";

                _.test("equal", [=](const container& c) {
                        polysync::tree tree = polysync::tree("type", {
                                { "start_time", std::uint16_t { 1 } },
                                { "scanner_info_list", std::vector<polysync::tree> {
                                        polysync::tree("type", {
                                                { "device_id", std::uint8_t { 2 } },
                                                { "scanner_type", std::uint8_t { 3 } },
                                                }),
                                        polysync::tree("type", {
                                                { "device_id", std::uint8_t { 4 } },
                                                { "scanner_type", std::uint8_t { 5 } },
                                                }),
                                        }}
                                });
                        std::string enc = encode_tree(tree, c);
                        expect(enc, equal_to(truth));
                        });

                _.test("not_equal", [=](const container& c) {
                        polysync::tree tree = polysync::tree("type", {
                                { "start_time", std::uint16_t { 1 } },
                                { "scanner_info_list", std::vector<polysync::tree> {
                                        polysync::tree("type", {
                                                { "device_id", std::uint8_t { 2 } },
                                                { "scanner_type", std::uint8_t { 3 } },
                                                }),
                                        polysync::tree("type", {
                                                { "device_id", std::uint8_t { 4 } },
                                                { "scanner_type", std::uint8_t { 6 } },
                                                }),
                                        }}});
                        std::string enc = encode_tree(tree, c);
                        expect(enc, not_equal_to(truth));
                        });

                _.test("missing_array", [=](const container& c) {
                        polysync::tree tree = polysync::tree("type", {
                                { "start_time", std::uint16_t { 1 } },
                                });
                        expect([=]() { encode_tree(tree, c); },
                                thrown<polysync::error>("field \"scanner_info_list\" not found in tree"));
                        });

        });

        _.subsuite("order", [](auto&_) {
                _.test("same", [=]() {

                        polysync::tree tree = polysync::tree("type", {
                                { "dest_guid", plog::ps_guid { 1 } },
                                { "data_type", std::uint32_t { 2 } },
                                { "payload", std::uint32_t { 3 } }
                                });

                        std::stringstream stream;
                        plog::encoder encode(stream);
                        encode(tree, descriptor::ps_byte_array_msg);

                        expect(stream.str(), bits("0100000000000000" "02000000" "03000000"));
                        });

                _.test("swapped", [=]() {
                        polysync::tree tree = polysync::tree("type", {
                                { "data_type", std::uint32_t { 2 } },
                                { "payload", std::uint32_t { 3 }  },
                                { "dest_guid", plog::ps_guid { 1 } },
                                });

                        std::stringstream stream;
                        plog::encoder encode(stream);
                        encode(tree, descriptor::ps_byte_array_msg);

                        expect(stream.str(), bits("0100000000000000" "02000000" "03000000"));
                        });

                _.test("skip-same", [=]() {

                        polysync::tree tree = polysync::tree("type", {
                                { "magic", std::uint32_t { 1 }, },
                                { "prev_size", std::uint32_t { 2 } },
                                { "size", std::uint8_t { 3 } },
                                { "skip-1", polysync::bytes { 0, 0, 0, 0 } },
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

                        polysync::tree tree = polysync::tree("type", {
                                { "magic", std::uint32_t { 1 } },
                                { "data_type", std::uint16_t { 5 } },
                                { "prev_size", std::uint32_t { 2 } },
                                { "size", std::uint8_t { 3 } },
                                { "skip-1", polysync::bytes { 0, 0, 0, 0 } },
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


