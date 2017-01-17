#include <mettle.hpp>

#include <polysync/descriptor.hpp>
#include <polysync/logging.hpp>
#include <polysync/hana_descriptor.hpp>
#include <polysync/descriptor/print.hpp>
#include <polysync/console.hpp>
#include <polysync/plog/core.hpp>

using namespace mettle;
namespace plog = polysync::plog;

// The TomlTable is a unit test fixture that parses a TOML string and provides a table structure.
struct TomlTable : std::shared_ptr<cpptoml::table>
{
    TomlTable( const std::string& toml )
    {
        std::istringstream stream( toml );
        cpptoml::parser parser( stream );
        std::shared_ptr<cpptoml::table> root = parser.parse();
        this->swap( root );
    }
};

// has_key is a custom mettle matcher to check in the TomlTable has a key.
// This is described in the mettle docs, the "User Guide", under the section
// "Writing Your Own Matchers".
struct has_key : mettle::matcher_tag
{
    has_key( const std::string& key ) : key(key) {}

    template <typename T, typename V>
    bool operator()( const std::map<T, V>& map ) const
    {
        return map.count(key);
    }

    bool operator()( std::shared_ptr<cpptoml::base> base ) const
    {
        if (!base->is_table()) {
            return false;
        }
        return base->as_table()->contains(key);
    };

    std::string desc() const
    {
        return "has_key \"" + key + "\"";
    }

    const std::string key;
};

constexpr char const* ps_byte_array_msg_TomlString = R"toml(
[ps_byte_array_msg]
    description = [
        { name = "dest_guid", type = "uint64" },
        { name = "data_type", type = "uint32" },
        { name = "payload", type = "uint32" }
    ]
    detector = { ibeo.header = { data_type = "160" } }
)toml";

constexpr char const* ps_msg_header_TomlString = R"toml(
[ps_msg_header]
    description = [
        { name = "type", type = "uint32" },
        { name = "timestamp", type = "uint64" },
        { name = "src_guid", type = "uint64" }
    ]
)toml";

polysync::descriptor::Type ps_byte_array_msg_TypeDescription {
    "ps_byte_array_msg", {
        { "dest_guid", typeid(plog::ps_guid) },
        { "data_type", typeid(uint32_t) },
        { "payload", typeid(uint32_t) }
     }
};

mettle::suite<TomlTable> description( "descriptor::Type", mettle::bind_factory(ps_byte_array_msg_TomlString),
        [](auto& _) {

        plog::load();
        polysync::logging::setLevels({ "debug2" });

        namespace descriptor = polysync::descriptor;

        _.subsuite( "construction", [](auto&_) {

                descriptor::Type truth { "ps_msg_header", {
                    { "type", typeid(plog::ps_msg_type) },
                    { "timestamp", typeid(plog::ps_timestamp) },
                    { "src_guid", typeid(plog::ps_guid) },
                }};

                _.test( "C++-inline", [truth](auto&_) {

                        descriptor::Type desc { "ps_msg_header", {
                            { "type", typeid(plog::ps_msg_type) },
                            { "timestamp", typeid(plog::ps_timestamp) },
                            { "src_guid", typeid(plog::ps_guid) },
                        }};

                        expect( desc, equal_to(truth) );

                        });

                _.test( "from boost::hana", [truth](auto&_) {

                        descriptor::Type desc =
                            descriptor::describe<plog::ps_msg_header>::type("ps_msg_header");

                        expect( desc, equal_to(truth) );

                        });

                _.test( "from TOML", [truth](auto&_) {

                        TomlTable table( ps_msg_header_TomlString );

                        std::vector<descriptor::Type> descriptions = descriptor::fromToml( table );

                        expect( descriptions.front(), equal_to(truth) );

                        });

                }); // subsuite construction

        _.subsuite( "printers", [](auto&_) {

                descriptor::Type ps_msg_header { "ps_msg_header", {
                    { "type", typeid(plog::ps_msg_type) },
                    { "timestamp", typeid(plog::ps_timestamp) },
                    { "src_guid", typeid(plog::ps_guid) },
                }};

                _.test( "console", [ps_msg_header](auto&_) {

                        std::string truth = "ps_msg_header: { "
                            "type: uint32, timestamp: uint64, src_guid: uint64 }";

                        // Get rid of color escapes so the string comparison can pass
                        polysync::format = std::make_shared<polysync::formatter::plain>();

                        std::stringstream os;
                        os << ps_msg_header;

                        expect( os.str(), equal_to(truth) );
                        });
                });

        _.subsuite( "equality-operators", [](auto&_) {

                _.test( "equal", []( TomlTable& ) {

                        polysync::descriptor::Type desc { "ps_byte_array_msg", {
                            { "dest_guid", typeid(plog::ps_guid) },
                            { "data_type", typeid(uint32_t) },
                            { "payload", typeid(uint32_t) } }
                            };

                            // exercises operator==(const descriptor::Type&, const descriptor::Type&)
                            expect( desc, equal_to(ps_byte_array_msg_TypeDescription) );
                });

                _.test( "name-mismatch", []( TomlTable& ) {

                        polysync::descriptor::Type desc { "wrong_name", {
                            { "dest_guid", typeid(plog::ps_guid) },
                            { "data_type", typeid(uint32_t) },
                            { "payload", typeid(uint32_t) } }
                            };

                            // exercises operator!=(const descriptor::type&, const descriptor::type&)
                            expect( desc, not_equal_to(ps_byte_array_msg_TypeDescription) );
                });

                _.test( "field-name-mismatch", []( TomlTable& ) {

                        polysync::descriptor::Type desc { "ps_byte_array_msg", {
                            { "dest_guid", typeid(plog::ps_guid) },
                            { "wrong_name", typeid(uint32_t) },
                            { "payload", typeid(uint32_t) } }
                            };

                            expect( desc, not_equal_to(ps_byte_array_msg_TypeDescription) );
                     });

                _.test( "field-type-mismatch", []( TomlTable& ) {

                        polysync::descriptor::Type desc { "ps_byte_array_msg", {
                            { "dest_guid", typeid(plog::ps_guid) },
                            { "data_type", typeid(double) }, // wrong type
                            { "payload", typeid(uint32_t) } }
                            };

                            expect( desc, not_equal_to(ps_byte_array_msg_TypeDescription) );
                });
        }); // subsuite equality-operator

}); // suite descriptor::Type

mettle::suite<TomlTable> toml( "TOML", mettle::bind_factory(ps_byte_array_msg_TomlString),
        [](auto& _) {

        plog::load();

        _.test( "check root", [](TomlTable& root) {
                expect( "is_table()", root->is_table(), equal_to(true) );
                expect( "is_array()", root->is_array(), equal_to(false) );
                expect( "is_table_array()", root->is_table_array(), equal_to(false) );
                expect( "empty", root->empty(), equal_to(false) );
                expect( "contains description", root->contains("description"), equal_to(false) );
                expect( "contains detector", root->contains("detector"), equal_to(false) );
                expect( "contains garbage", root->contains("Vanilla Ice"), equal_to(false) );
                });

        _.test( "check type-table", [](TomlTable& root) {
                auto base = root->get("ps_byte_array_msg");
                expect( "is table", base->is_table(), equal_to(true) );
                expect( "is array", base->is_array(), equal_to(false) );
                expect( "is table_array", base->is_table_array(), equal_to(false) );
                expect( "is value", base->is_value(), equal_to(false) );
                expect( "is array", base->is_array(), equal_to(false) );
                expect( "is table_array", base->is_table_array(), equal_to(false) );

                std::shared_ptr<cpptoml::table> table = base->as_table();
                expect( "is_table()", table->is_table(), equal_to(true) );
                expect( "is_array()", table->is_array(), equal_to(false) );
                expect( "is_table_array()", table->is_table_array(), equal_to(false) );
                expect( "empty", table->empty(), equal_to(false) );
                expect( "contains description", table->contains("description"), equal_to(true) );
                expect( "contains detector", table->contains("detector"), equal_to(true) );
                expect( "contains garbage", table->contains("Vanilla Ice"), equal_to(false) );
                });

        _.test( "check description", [](TomlTable& root) {
                std::shared_ptr<cpptoml::table> type = root->get_table("ps_byte_array_msg");
                auto base = type->get("description");
                expect( "is_table()", base->is_table(), equal_to(false) );
                expect( "is_array()", base->is_table(), equal_to(false) );
                expect( "is_table_array()", base->is_table_array(), equal_to(true) );

                std::shared_ptr<cpptoml::table_array> desc = base->as_table_array();
                expect( desc->get(), each( has_key("name") ) );
                expect( desc->get(), each( has_key("type") ) );

                });

});
