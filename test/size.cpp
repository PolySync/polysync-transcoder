#include <polysync/description.hpp>
#include <polysync/plog/core.hpp>

#include <mettle.hpp>

using namespace mettle;
namespace plog = polysync::plog;

mettle::suite<> size("size", [](auto& suite) {
        suite.test("log_record", []() {
                expect(polysync::descriptor::size<plog::log_record>::value(), equal_to(20));
                });
        });

