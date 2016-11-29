#include <polysync/detector.hpp>
#include <polysync/console.hpp>
#include <polysync/print_hana.hpp>

namespace std {

std::string to_printable(const std::exception& e) {
    const polysync::error* pe = dynamic_cast<const polysync::error *>(&e);
    if (pe) {
        std::stringstream os;
        os << *pe;
        return os.str();
    }
    else
        return e.what();
}

}

#include <mettle.hpp>
#include "types.hpp"

namespace plog = polysync::plog;

// Define a fixture to parse a TOML string
struct fixture {
    std::istringstream stream;
    cpptoml::parser parser;
    std::shared_ptr<cpptoml::table> root;

    fixture(const std::string& toml) : 
        stream(toml), parser(stream), root(parser.parse()) {}

    std::shared_ptr<cpptoml::table> operator->() const { return root; };
};

mettle::suite<fixture> detect("detect", mettle::bind_factory(toml::ps_byte_array_msg), [](auto& _) {
        _.teardown([](fixture&) {
                polysync::descriptor::catalog.clear();
                });

        // _.test("detector", [](fixture& root) {
        //         auto table = root->get_table("ps_byte_array_msg");
        //         polysync::descriptor::catalog.emplace(
        //                 "ps_byte_array_msg", descriptor::ps_byte_array_msg);

        //         auto it = std::find_if(polysync::detector::catalog.begin(), polysync::detector::catalog.end(), 
        //                     [](auto d) { return d.parent == "ps_byte_array_msg"; });

        //         expect(*it, mettle::array( 
        //                     descriptor::type { "dest_guid" } 
        //                     ));
        //         });

        });


