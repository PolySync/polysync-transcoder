#include <mettle.hpp>

#include <polysync/plog/decoder.hpp>
#include <polysync/console.hpp>
#include "types.hpp"

using namespace mettle;
namespace plog = polysync::plog;

// Instantiate the static console format; this is used inside of mettle to
// print failure messages through operator<<'s defined in io.hpp.
namespace polysync { namespace console { codes format = color(); }}

mettle::suite<> decode("decode_description", [](auto& _) {
        
        _.test("test", []() {
                });
        });


