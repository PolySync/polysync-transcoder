#include <mettle.hpp>

#include <polysync/plog/decoder.hpp>

using namespace mettle;
namespace plog = polysync::plog;

polysync::tree decode_hex( const polysync::descriptor::Type& desc,
		           const std::string& hexblob ) {

    // Form a binary blob from a hex string with the help of boost::multiprecision
    namespace mp = boost::multiprecision;
    std::string blob;
    mp::export_bits( mp::cpp_int( "0x" + hexblob), std::back_inserter(blob), 8 );

    // decode a polysync::tree from the description
    std::stringstream stream(blob);
    plog::decoder serializer(stream);
    polysync::tree result = *serializer(desc).target<polysync::tree>();

    // Augment the result with any extra undecoded bits in the blob
    if ( stream.tellg() < blob.size() ) {
        result->emplace_back("raw", serializer.decode<polysync::bytes>());
    }

    return result;
}

mettle::suite<> decode("plog::decode", [](auto& _) {

        _.teardown([]() {
                polysync::descriptor::catalog.clear();
	    });

        _.test( "native_types", []() {

		polysync::descriptor::Type ps_byte_array_msg { "ps_byte_array_msg", {
			{ "dest_guid", typeid(plog::ps_guid) },
			{ "data_type", typeid(uint16_t) },
			{ "payload", typeid(uint32_t) }
			} };

                polysync::tree truth = polysync::tree( "ps_byte_array_msg", {
                        { "dest_guid", plog::ps_guid { 1 } },
                        { "data_type", std::uint16_t { 2 } },
                        { "payload", std::uint32_t { 3 } }
			});

                std::string hexblob = "0100000000000000" "0200" "03000000";

                polysync::tree tree = decode_hex( ps_byte_array_msg, hexblob );
                expect( tree, equal_to(truth) );

                });

        _.test( "reserved_bytes", []() {

		polysync::descriptor::Type ibeo_header { "ibeo.header", {
			{ "magic", typeid(std::uint32_t) },
			{ "prev_size", typeid(std::uint32_t) },
			{ "size", typeid(std::uint8_t) },
			{ "skip-1", polysync::descriptor::Skip { 4, 1 } },
			{ "device_id", typeid(std::uint8_t) },
			{ "data_type", typeid(std::uint16_t) },
			{ "time", typeid(std::uint64_t) } }
		};

                polysync::tree truth = polysync::tree( "ibeo.header", {
                        { "magic", std::uint32_t { 1 } },
                        { "prev_size", std::uint32_t { 2 } },
                        { "size", std::uint8_t { 3 } },
                        { "skip-1", polysync::bytes { 0xDE, 0xAD, 0xBE, 0xEF } },
                        { "device_id", std::uint8_t { 4 } },
                        { "data_type", std::uint16_t { 5 } },
                        { "time", std::uint64_t { 6 } },
                    });

                std::string hexblob =
                    "01000000" "02000000" "03"
                    "DEADBEEF" // 4 byte skip
                    "04" "0500" "0600000000000000";

                polysync::tree tree = decode_hex( ibeo_header, hexblob );
                expect( tree, equal_to(truth) );

                });

        _.test( "unknown_type", []() {

                polysync::descriptor::Type desc { "unknown_type", {
                    { "dest_guid", typeid(plog::ps_guid) },
                    { "data_type", typeid(std::uint32_t) },
                    { "time", typeid(void) } // the unknown type
		    }
		    };

                std::string hexblob = "0100000000000000" "02000000" "0300000000000000";

                expect([=]() { decode_hex( desc, hexblob ); },
                        thrown<polysync::error>( "no typemap" ));

                });

        _.subsuite( "nested_type", [](auto&_) {

                polysync::descriptor::Type scanner_info { "scanner_info", {
                    { "device_id", typeid(std::uint8_t) },
                    { "scanner_type", typeid(std::uint8_t) } }
                };
                polysync::descriptor::catalog.emplace("scanner_info", scanner_info);
                polysync::descriptor::Type container { "container", {
                    { "start_time", typeid(std::uint16_t) },
                    { "scanner_info", polysync::descriptor::Nested{"scanner_info"} } }
                };

                polysync::tree truth = polysync::tree("container", {
                        { "start_time", std::uint16_t { 1 } },
                        { "scanner_info", polysync::tree("scanner_info", {
                                { "device_id", std::uint8_t { 2 } },
                                { "scanner_type", std::uint8_t { 3 } }
                                }) }
                    });

                _.test( "equal", [=]() {
                        polysync::tree correct = decode_hex(container, "0100" "02" "03");
                        expect(correct, equal_to(truth));
                        });

                _.test( "notequal", [=]() {
                        polysync::tree wrong_value = decode_hex(container, "0100" "02" "04");
                        expect( wrong_value, not_equal_to(truth) );
                        });

                _.test( "short_read", [=]() {
                        expect([=]() { decode_hex(container, "0100" "02"); },
                                thrown<polysync::error>("read error"));
                        });

                _.test( "undecoded_bits", [=]() {
                        polysync::tree toolong = decode_hex(container, "0100" "02" "03" "04");
                        expect(toolong, not_equal_to(truth));
                        });
            });

        _.subsuite( "terminal_array", [](auto &_) {

                polysync::descriptor::Type sometype { "type", {
                    { "time", typeid(std::uint16_t) },
                    { "data", polysync::descriptor::Array { 3, typeid(std::uint8_t) } }
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
                        expect([=]() { decode_hex(sometype, "0100" "02" "03"); },
                                thrown<polysync::error>("read error"));
                        });

                _.test("toolong", [=]() {
                        polysync::tree toolong = decode_hex(sometype, "0100" "02" "03" "04" "05");
                        expect(toolong, not_equal_to(truth));
                        });

            });

        _.subsuite( "nested_array", [](auto &_) {
                _.test("equal", [=]() {
			polysync::descriptor::Type scanner_info { "scanner_info", {
			    { "device_id", typeid(std::uint8_t) },
			    { "scanner_type", typeid(std::uint8_t) }
			} };

			polysync::descriptor::catalog.emplace("scanner_info", scanner_info);
			polysync::descriptor::Type container { "container", {
			    { "time", typeid(std::uint16_t) },
			    { "scanner_info", polysync::descriptor::Array { 3, "scanner_info" } } }
			};

                polysync::tree truth = polysync::tree( "container", {
                        { "time", std::uint16_t { 1 } },
                        { "scanner_info", std::vector<polysync::tree> {
                                polysync::tree("scanner_info", {
                                        { "device_id", std::uint8_t { 2 } },
                                        { "scanner_type", std::uint8_t { 3 } },
                                        }),
                                polysync::tree("scanner_info", {
                                        { "device_id", std::uint8_t { 4 } },
                                        { "scanner_type", std::uint8_t { 5 } },
                                        }),
                                polysync::tree("scanner_info", {
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

        _.subsuite( "dynamic_array", [](auto &_) {

                polysync::descriptor::Type sometype { "type", {
                    { "points", typeid(std::uint16_t) },
                    { "data", polysync::descriptor::Array { "points", typeid(std::uint8_t) } }
                } };

                polysync::tree truth = polysync::tree( "type", {
                        { "points", std::uint16_t { 3 } },
                        { "data", std::vector<uint8_t> { 2, 3, 4 } }
                    });

                _.test( "equal", [=]() {
                        polysync::tree correct = decode_hex(sometype, "0300" "02" "03" "04");
                        expect(correct, equal_to(truth));
                        });

            });

});

