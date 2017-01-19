#include <mettle.hpp>

#include <polysync/detector.hpp>
#include <polysync/descriptor.hpp>
#include <polysync/print_tree.hpp>

using namespace mettle;

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

namespace polysync { namespace detector {

extern polysync::variant parseTerminalFromString( const std::string&, const std::type_index& );

}} // namespace polysync::detector

// Mettle is able to instantiate a suite template for a list of types.  The
// number_factory is a fixture that teaches mettle how to construct a test
// value for any tested type.
struct number_factory {
    int value;

    template <typename T>
    polysync::variant make() {
        return boost::numeric_cast<T>( value );
    }
};


mettle::suite<>
parseTerminal( "parseTerminalFromString", [](auto& _) {

        using polysync::detector::parseTerminalFromString;

        mettle::subsuite<
            std::int8_t, std::int16_t, std::int32_t, std::int64_t,
            std::uint8_t, std::uint16_t, std::uint32_t, std::uint64_t
            > ( _, "integer", [](auto& _) {
                    using T = mettle::fixture_type_t< decltype(_) >;

                    _.test( "decimal", []( polysync::variant var ) {
                            expect( parseTerminalFromString( "42", typeid(T) ),
                                    equal_to( T { 42 } ) );
                            });

                    _.test( "hex", []( polysync::variant var ) {
                            expect( parseTerminalFromString( "0x42", typeid(T) ),
                                    equal_to( T { 0x42 } ) );
                            });

                    _.test( "octal", []( polysync::variant var ) {
                            expect( parseTerminalFromString( "042", typeid(T) ),
                                    equal_to( T { 042 } ) );
                            });
                    });

        mettle::subsuite< float, double >( _, "floatingpoint", [](auto& _) {
                    using T = mettle::fixture_type_t< decltype(_) >;

                    _.test( "parse", []( polysync::variant var ) {
                            expect( parseTerminalFromString( "42", typeid(T) ),
                                    equal_to( T { 42 } ) );
                            });
                    });

        _.test("invalid_type", []() {
                expect([]() { parseTerminalFromString( "42", typeid(std::string) ); },
                        thrown<polysync::error>( "no string converter defined for terminal type" ));
                });

        mettle::subsuite<
            std::int8_t, std::int16_t, std::int32_t,
            std::uint8_t, std::uint16_t, std::uint32_t
            > ( _, "overflow", [](auto& _) {
                    using T = mettle::fixture_type_t< decltype(_) >;

                    _.test( "hex", []( polysync::variant var ) {
                            expect( []() { parseTerminalFromString( "0xFFFFFFFFFFFFFFFF", typeid(T) ); },
                                    thrown<polysync::error>( "value overflow" ) );
                            });
                    });

            });

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

mettle::suite<> buildDetectors( "detector::buildDetectors", [](auto& _) {

        _.setup([]() {
                polysync::descriptor::catalog.clear();
                polysync::detector::catalog.clear();

                polysync::descriptor::Type current { "current", {
                    { "field", typeid(std::uint32_t) }
                }};

                polysync::descriptor::Type next { "next", {
                    { "field", typeid(std::uint32_t) }
                }};

                polysync::descriptor::catalog.emplace( "current", current );
                polysync::descriptor::catalog.emplace( "next", next );

                });

        _.teardown([]() {
                polysync::descriptor::catalog.clear();
                polysync::detector::catalog.clear();
                });

        _.test( "missing_name", []() {
                expect( []() {
                        TomlTable table( R"toml( detector = [ { junk = "42" } ])toml" );

                        polysync::detector::buildDetectors(
                                "current",
                                table->get("detector")->as_table_array()
                                );

                        }, thrown<polysync::error>( "detector requires a \"name\" field" ));
                });

        _.test( "wrong_name", []() {
                expect( []() {

                        TomlTable table( R"toml( detector = [ { name = 42 } ])toml" );

                        polysync::detector::buildDetectors(
                                "current",
                                table->get("detector")->as_table_array()
                                );

                        }, thrown<polysync::error>( "detector precursor must be a string" ));
                });

        _.test( "wrong_field", []() {
                expect( []() {
                        TomlTable table( R"toml( detector = [ { name = "next", rainbows = 42 } ])toml" );

                        polysync::detector::buildDetectors(
                                "current",
                                table->get("detector")->as_table_array()
                                );

                        }, thrown<polysync::error>( "type description lacks detector field" ));
                });
});

mettle::suite<> loadCatalog( "detector::loadCatalog", [](auto& _) {

        _.setup([]() {
                polysync::descriptor::catalog.clear();
                polysync::detector::catalog.clear();
                });

        _.teardown([]() {
                polysync::descriptor::catalog.clear();
                polysync::detector::catalog.clear();
                });


        _.test( "missing_description", []() {
                expect( []() {
                        TomlTable table( R"toml( detector = [ { name = "unicorn" } ])toml" );

                        polysync::detector::loadCatalog( "current", table );

                        }, thrown<polysync::error>( "requested detector lacks corresponding type description" ));
                });

        _.test( "not_a_table", []() {

                polysync::descriptor::Type current { "current", {
                    { "field", typeid(std::uint32_t) }
                }};

                polysync::descriptor::catalog.emplace( "current", current );

                expect( []() {
                        TomlTable table( R"toml( detector = [ "next" ])toml" );

                        polysync::detector::loadCatalog( "current", table );

                        }, thrown<polysync::error>( "detector list must be an array" ));
                });

        _.test( "ambiguity", []() {

                polysync::descriptor::Type current { "current", {
                    { "field", typeid(std::uint32_t) }
                }};

                polysync::descriptor::Type next { "next", {
                    { "field", typeid(std::uint32_t) }
                }};

                polysync::descriptor::catalog.emplace( "current", current );
                polysync::descriptor::catalog.emplace( "next", next );

                TomlTable table( R"toml( detector = [ { name = "current", field = "42" } ])toml" );

                polysync::detector::loadCatalog( "next", table );

                expect( [&]() {
                        polysync::detector::loadCatalog( "next", table );
                        }, thrown<polysync::error>( "ambiguous detector" ));
                });

        mettle::subsuite<>( _, "loaded_descriptors", [](auto& _) {
                _.setup([]() {

                        polysync::descriptor::Type current { "current", {
                            { "key", typeid(std::uint32_t) },
                            { "nest", polysync::descriptor::Nested { "nested" } }
                        }};

                        polysync::descriptor::Type next { "next", {
                            { "field", typeid(std::uint32_t) }
                        }};

                        polysync::descriptor::Type nested { "nested", {
                            { "field", typeid(std::uint32_t) }
                        }};

                        polysync::descriptor::catalog.emplace( "current", current );
                        polysync::descriptor::catalog.emplace( "next", next );
                        polysync::descriptor::catalog.emplace( "nested", nested );

                        });

                _.test( "string_key", []() {

                        constexpr const char* toml = R"toml(
                            description = [ { name = "next", type = "uint32" } ]
                            detector = [ { name = "current", key = "42" } ]
                        )toml";

                        TomlTable table( toml );
                        polysync::detector::loadCatalog( "next", table );

                        });

                _.test( "integer_key", []() {

                        constexpr const char* toml = R"toml(
                            description = [ { name = "next", type = "uint32" } ]
                            detector = [ { name = "current", key = 42 } ]
                        )toml";

                        expect([ toml ]() {
                                TomlTable table( toml );
                                polysync::detector::loadCatalog( "current", table );
                                }, thrown<polysync::error>(
                                    "detector value must be represented as a string" ));

                        });

                _.test( "nested_key", []() {

                        constexpr const char* toml = R"toml(
                            description = [ { name = "next", type = "uint32" } ]
                            detector = [ { name = "current", nest = "42" } ]
                        )toml";

                        expect([ toml ]() {
                                TomlTable table( toml );
                                polysync::detector::loadCatalog( "current", table );
                                }, thrown<polysync::error>( "illegal key on compound type" ));
                        });
        });
});

