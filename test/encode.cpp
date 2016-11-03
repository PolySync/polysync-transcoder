#include <mettle.hpp>

#include <polysync/plog/encoder.hpp>
#include <polysync/console.hpp>
#include "types.hpp"

using namespace mettle;
namespace plog = polysync::plog;

// Instantiate the static console format; this is used inside of mettle to
// print failure messages through operator<<'s defined in io.hpp.
namespace polysync { namespace console { codes format = color(); }}

mettle::suite<> decode("plog::encode", [](auto& _) {
        
        _.test("in-order", [=]() {

                plog::tree tree = plog::tree::create({
                        { plog::guid { 1 }, "dest_guid" },
                        { std::uint32_t { 2 }, "data_type" },
                        { std::uint32_t { 3 }, "payload" }
                    });

                std::stringstream stream;
                plog::encoder encode(stream);
                encode(tree, descriptor::ps_byte_array_msg);

                expect(stream.str(), bits("0100000000000000" "02000000" "03000000"));
                });

        _.test("swapped-order", [=]() {
                plog::tree tree = plog::tree::create({
                        { std::uint32_t { 2 }, "data_type" },
                        { std::uint32_t { 3 }, "payload" },
                        { plog::guid { 1 }, "dest_guid" },
                    });

                std::stringstream stream;
                plog::encoder encode(stream);
                encode(tree, descriptor::ps_byte_array_msg);

                expect(stream.str(), bits("0100000000000000" "02000000" "03000000"));
                });

        _.test("skip-in-order", [=]() {

                plog::tree tree = plog::tree::create({
                        { std::uint32_t { 1 }, "magic" },
                        { std::uint32_t { 2 }, "prev_size" },
                        { std::uint8_t { 3 }, "size" },
                        { std::uint8_t { 4 }, "device_id" },
                        { std::uint16_t { 5 }, "data_type" },
                        { std::uint64_t { 6 }, "time" },
                    });

                std::stringstream stream;
                plog::encoder encode(stream);
                encode(tree, descriptor::ibeo_header);

                expect(stream.str(), 
                        bits("01000000" "02000000" "03" 
                             "00000000" // 4 byte skip
                             "04" "0500" "0600000000000000"));
                });

        _.test("skip-swapped-order", [=]() {

                plog::tree tree = plog::tree::create({
                        { std::uint32_t { 1 }, "magic" },
                        { std::uint16_t { 5 }, "data_type" },
                        { std::uint32_t { 2 }, "prev_size" },
                        { std::uint8_t { 3 }, "size" },
                        { std::uint8_t { 4 }, "device_id" },
                        { std::uint64_t { 6 }, "time" },
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


