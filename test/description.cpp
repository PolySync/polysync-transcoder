#include <mettle.hpp>

#include <polysync/description.hpp>
#include <polysync/console.hpp>
#include "types.hpp"

using namespace mettle;
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

mettle::suite<fixture> description("description", mettle::bind_factory(toml::ps_byte_array_msg),
        [](auto& _) {

        _.subsuite("type", [](auto&_) {
                _.test("equal", [](fixture&) {
                        polysync::descriptor::type desc { "ps_byte_array_msg", {
                            { "dest_guid", typeid(plog::guid) },
                            { "data_type", typeid(uint32_t) },
                            { "payload", typeid(uint32_t) } }
                            };

                            expect(desc, equal_to(descriptor::ps_byte_array_msg));
                });

                _.test("name-mismatch", { mettle::skip }, [](fixture&) {
                        polysync::descriptor::type desc { "byte_array_msg", {
                            { "dest_guid", typeid(plog::guid) },
                            { "data_type", typeid(uint32_t) },
                            { "payload", typeid(uint32_t) } }
                            };

                            // not_equal_to thinks the arg is
                            // descriptor::field, not descriptor::type and call
                            // the wrong comparison operator.  WTF?  I do not
                            // understand this bug at all.
                            expect(desc, not_equal_to(descriptor::ps_byte_array_msg));
                });

                _.test("field-name-mismatch", [](fixture&) {
                        polysync::descriptor::type desc { "byte_array_msg", {
                            { "dest_guid", typeid(plog::guid) },
                            { "type", typeid(uint32_t) },
                            { "payload", typeid(uint32_t) } }
                            };

                            expect(desc, not_equal_to(descriptor::ps_byte_array_msg));
                });

                _.test("field-type-mismatch", [](fixture&) {
                        polysync::descriptor::type desc { "byte_array_msg", {
                            { "dest_guid", typeid(plog::guid) },
                            { "data_type", typeid(uint16_t) },
                            { "payload", typeid(uint32_t) } }
                            };

                            expect(desc, not_equal_to(descriptor::ps_byte_array_msg));
                });
        });

        _.subsuite("read-TOML", [](auto&_) {

                _.test("root", [](fixture& root) {
                        expect("is_table()", root->is_table(), equal_to(true));
                        expect("is_array()", root->is_array(), equal_to(false));
                        expect("is_table_array()", root->is_table_array(), equal_to(false));
                        expect("empty", root->empty(), equal_to(false));
                        expect("contains description", root->contains("description"), equal_to(false));
                        expect("contains detector", root->contains("detector"), equal_to(false));
                        expect("contains garbage", root->contains("Vanilla Ice"), equal_to(false));
                        });

                _.test("type table", [](fixture& root) {
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

                _.test("description", [](fixture& root) {
                        std::shared_ptr<cpptoml::table> type = root->get_table("ps_byte_array_msg");
                        auto base = type->get("description");
                        expect("is_table()", base->is_table(), equal_to(false));
                        expect("is_array()", base->is_table(), equal_to(false));
                        expect("is_table_array()", base->is_table_array(), equal_to(true));

                        std::shared_ptr<cpptoml::table_array> desc = base->as_table_array();
                        expect(desc->get(), each(has_key("name")));
                        expect(desc->get(), each(has_key("type")));

                        });
        });

        _.subsuite("build-description", [](auto&_) {
                _.test("TOML", [](fixture& root) {
                        polysync::descriptor::catalog_type catalog;
                        polysync::descriptor::load(
                                "ps_byte_array_msg", root->get_table("ps_byte_array_msg"), catalog);
                        expect(catalog, has_key("ps_byte_array_msg"));

                        const polysync::descriptor::type& desc = catalog.at("ps_byte_array_msg");
                        expect(desc, array(
                                    polysync::descriptor::field { "dest_guid", typeid(plog::guid) },
                                    polysync::descriptor::field { "data_type", typeid(std::uint32_t) },
                                    polysync::descriptor::field { "payload", typeid(std::uint32_t) }
                                    ));
                        });

                });

                _.test("static", [](auto&_) {
                        polysync::descriptor::describe<plog::msg_header> desc;
                        polysync::descriptor::type truth { "msg_header", {
                            { "type", typeid(plog::msg_type) },
                            { "timestamp", typeid(plog::timestamp) },
                            { "src_guid", typeid(plog::guid) },
                        }};
                        expect(desc.type(), equal_to(truth));
                        });

                _.test("string", [](auto&_) {
                        polysync::descriptor::describe<plog::msg_header> desc;
                        std::string truth = "msg_header { "
                            "type: uint32; timestamp: uint64; src_guid: uint64; }";
                        expect(desc.string(), equal_to(truth));
                        });
        });
