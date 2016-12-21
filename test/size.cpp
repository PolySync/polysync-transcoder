#include <polysync/size.hpp>
#include <polysync/plog/core.hpp>

#include <mettle.hpp>

using namespace mettle;
namespace plog = polysync::plog;

mettle::suite<> size("size", [](auto& suite) {
        suite.test("log_record", []() {
                expect(polysync::size<plog::ps_log_record>::value(), equal_to(20));
                });
        });

