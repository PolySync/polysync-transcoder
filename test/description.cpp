#include <mettle.hpp>

#include <polysync/plog/description.hpp>
#include <polysync/console.hpp>
#include <polysync/plog/io.hpp>
#include "types.hpp"

using namespace mettle;
namespace plog = polysync::plog;

// Instantiate the static console format; this is used inside of mettle to
// print failure messages through operator<<'s defined in io.hpp.
namespace polysync { namespace console { codes format = color(); }}

// Define a fixture to parse a TOML string
struct fixture {
    std::istringstream stream;
    cpptoml::parser parser;
    std::shared_ptr<cpptoml::table> root;

    fixture(const std::string& toml) : 
        stream(toml), parser(stream), root(parser.parse()) {}

    std::shared_ptr<cpptoml::table> operator->() const { return root; };
};

mettle::suite<fixture> description("description", mettle::bind_factory(ps_byte_array_msg),
        [](auto& _) {

        _.test("TOML root", [](fixture& root) {
                expect("is_table()", root->is_table(), equal_to(true));
                expect("is_array()", root->is_array(), equal_to(false));
                expect("is_table_array()", root->is_table_array(), equal_to(false));
                expect("empty", root->empty(), equal_to(false));
                expect("contains description", root->contains("description"), equal_to(false));
                expect("contains detector", root->contains("detector"), equal_to(false));
                expect("contains garbage", root->contains("Vanilla Ice"), equal_to(false));
                });

        _.test("TOML type table", [](fixture& root) {
                auto base = root->get("ps_byte_array_msg");
                expect("is table", base->is_table(), equal_to(true));
                expect("is array", base->is_array(), equal_to(false));
                expect("is table_array", base->is_table_array(), equal_to(false));
                expect("is value", base->is_value(), equal_to(false));
                expect("is array", base->is_array(), equal_to(false));
                expect("is table_array", base->is_table_array(), equal_to(false));

                std::shared_ptr<cpptoml::table> table = base->as_table();
                expect("is_table()", table->is_table(), equal_to(true));
                expect("is_array()", table->is_array(), equal_to(false));
                expect("is_table_array()", table->is_table_array(), equal_to(false));
                expect("empty", table->empty(), equal_to(false));
                expect("contains description", table->contains("description"), equal_to(true));
                expect("contains detector", table->contains("detector"), equal_to(true));
                expect("contains garbage", table->contains("Vanilla Ice"), equal_to(false));
                });

        _.test("TOML description", [](fixture& root) {
                std::shared_ptr<cpptoml::table> type = root->get_table("ps_byte_array_msg");
                auto base = type->get("description");
                expect("is_table()", base->is_table(), equal_to(false));
                expect("is_array()", base->is_table(), equal_to(false));
                expect("is_table_array()", base->is_table_array(), equal_to(true));

                std::shared_ptr<cpptoml::table_array> desc = base->as_table_array();
                expect(desc->get(), each(has_key("name")));
                expect(desc->get(), each(has_key("type")));

                });

        _.test("plog::description", [](fixture& root) {
                plog::descriptor::catalog_type catalog;
                plog::descriptor::load(
                        "ps_byte_array_msg", root->get_table("ps_byte_array_msg"), catalog);
                expect(catalog, has_key("ps_byte_array_msg"));

                const plog::descriptor::type& desc = catalog.at("ps_byte_array_msg");
                expect(desc, array( 
                            plog::descriptor::field { "dest_guid", typeid(plog::guid) }, 
                            plog::descriptor::field { "data_type", typeid(std::uint32_t) },
                            plog::descriptor::field { "payload", typeid(std::uint32_t) }
                            ));
                });

        _.test("plog::descriptor", [](fixture& root) {
                plog::descriptor::catalog_type catalog;
                plog::descriptor::load(
                        "ps_byte_array_msg", root->get_table("ps_byte_array_msg"), catalog);
                expect(catalog, has_key("ps_byte_array_msg"));

                const plog::descriptor::type& desc = catalog.at("ps_byte_array_msg");
                expect(desc, array( 
                             plog::descriptor::field { "dest_guid", typeid(plog::guid) }, 
                             plog::descriptor::field { "data_type", typeid(std::uint32_t)},
                             plog::descriptor::field { "payload", typeid(std::uint32_t)}
                            ));
                });

        });
