#include <mettle.hpp>

#include <polysync/plog/detector.hpp>
#include <polysync/transcode/console.hpp>
#include <polysync/plog/io.hpp>
#include "types.hpp"

namespace plog = polysync::plog;

constexpr char const* ps_byte_array_msg = R"toml(
[ps_byte_array_msg]
    description = [
        { name = "dest_guid", type = "ps_guid" },
        { name = "data_type", type = "uint32" },
        { name = "payload", type = "uint32" }
    ]
    detector = { ibeo.header = { data_type = "160" } } 
)toml";

// Define a fixture to parse a TOML string
struct fixture {
    std::istringstream stream;
    cpptoml::parser parser;
    std::shared_ptr<cpptoml::table> root;

    fixture(const std::string& toml) : 
        stream(toml), parser(stream), root(parser.parse()) {}

    std::shared_ptr<cpptoml::table> operator->() const { return root; };
};

mettle::suite<fixture> detector("detector", mettle::bind_factory(ps_byte_array_msg), [](auto& _) {
        _.test("plog::detector", [](fixture& root) {
                plog::descriptor::catalog_type descriptors;
                plog::detector::catalog_type detectors;
                auto table = root->get_table("ps_byte_array_msg");
                plog::descriptor::load("ps_byte_array_msg", table, descriptors);
                plog::detector::load("ps_byte_array_msg", table, detectors);
                // expect(catalog, has_key("ps_byte_array_msg"));

                // const plog::detector::type& desc = catalog.at("ps_byte_array_msg");
                // expect(desc, array( 
                //             plog::descriptor::type { "dest_guid" } 
                //             ));
                });

        });


