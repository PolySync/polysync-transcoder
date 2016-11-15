#include <mettle.hpp>

#include <polysync/detector.hpp>
#include <polysync/console.hpp>
#include <polysync/print_hana.hpp>
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
        _.test("detector", [](fixture& root) {
                polysync::descriptor::catalog_type descriptors;
                polysync::detector::catalog_type detectors;
                auto table = root->get_table("ps_byte_array_msg");
                polysync::detector::load("ps_byte_array_msg", table, detectors);
                polysync::descriptor::load("ps_byte_array_msg", table, descriptors);
                // expect(descriptor::catalog, has_key("ps_byte_array_msg"));

                auto it = std::find_if(polysync::detector::catalog.begin(), polysync::detector::catalog.end(), 
                            [](auto d) { return d.parent == "ps_byte_array_msg"; });

                // expect(*it, mettle::array( 
                //             descriptor::type { "dest_guid" } 
                //             ));
                });

        });


