#include <polysync/transcode/description.hpp>
#include <polysync/transcode/io.hpp>

#include <mettle.hpp>

using namespace mettle;
namespace plog = polysync::plog;

mettle::suite<> size("size", [](auto& suite) {
        suite.test("log_record", []() {
                expect(plog::size<plog::log_record>::value(), equal_to(20));
                });
        });

