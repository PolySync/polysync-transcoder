#include <mettle.hpp>

#include <polysync/tree.hpp>
#include <polysync/console.hpp>

using namespace mettle;

mettle::suite<> print("print", [](auto& _) {

        _.test("vector<std::uint32_t>", []() {
                std::vector<std::uint32_t> raw { 1, 2, 3, 4 };
                std::stringstream os;
                os << raw;
                expect(os.str(), equal_to("[ 1 2 3 4 ] (4 elements)"));
                });

        // bytes=std::vector<uint8_t> printer is specialized
        _.test("bytes", []() {
                polysync::bytes raw { 1, 2, 3, 4 };
                std::stringstream os;
                os << raw;
                expect(os.str(), equal_to("[ 1 2 3 4 ] (4 elements)"));
                });

        _.subsuite("node", [](auto&_) {
                _.test("std::uint8_t", []() {
                        std::uint8_t scalar = 42;
                        std::stringstream os;
                        os << polysync::node("scalar", scalar);
                        expect(os.str(), equal_to("42"));
                        });

                _.test("std::uint32_t", []() {
                        std::uint32_t scalar = 42;
                        std::stringstream os;
                        os << polysync::node("scalar", scalar);
                        expect(os.str(), equal_to("42"));
                        });

                _.test("vector<std::uint32_t>", []() {
                        std::vector<std::uint32_t> raw { 1, 2, 3, 4 };
                        std::stringstream os;
                        os << polysync::node("vector", raw);
                        expect(os.str(), equal_to("[ 1 2 3 4 ] (4 elements)"));
                        });

                _.test("bytes", []() {
                        polysync::bytes raw { 1, 2, 3, 4 };
                        std::stringstream os;
                        os << polysync::node("vector", raw);
                        expect(os.str(), equal_to("[ 1 2 3 4 ] (4 elements)"));
                        });

               });

        });



