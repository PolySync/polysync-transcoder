#include <iostream>

#include <boost/hana/ext/std/integral_constant.hpp>

#include <mettle.hpp>

#include <polysync/tree.hpp>
#include <polysync/print_tree.hpp>

using namespace mettle;

// Mettle is able to instantiate a suite template for a list of types.  The
// number_factory is a fixture that teaches mettle how to construct a test
// value for any tested type.
struct number_factory {
    template <typename T>
    polysync::variant make() {
        return T { 42 };
    }
};

suite< 
    std::int8_t, std::int16_t, std::int32_t, std::int64_t,
    std::uint8_t, std::uint16_t, std::uint32_t, std::uint64_t,
    float, double,
    boost::multiprecision::cpp_int
    >
number("scalar_comparison", number_factory {}, [](auto& _) {

        using T = mettle::fixture_type_t< decltype(_) >;

        _.test( "equal", []( polysync::variant var ) {
                expect( var, equal_to( T { 42 } ) );
                });

        _.test( "not_equal", []( polysync::variant var ) {
                expect( var, not_equal_to(T { 41 } ) );
                });

        _.test( "type_mismatch", []( polysync::variant var ) {
                expect( var, not_equal_to( std::vector<std::uint8_t> { 42 } ) );
                });
        });

struct vector_factory {
    template <typename T>
    polysync::variant make() {
        return std::vector<T> { 17, 42, 37 };
    }
};

mettle::suite<
    std::int8_t, std::int16_t, std::int32_t, std::int64_t,
    std::uint8_t, std::uint16_t, std::uint32_t, std::uint64_t,
    float, double,
    boost::multiprecision::cpp_int
    >
terminal_array( "scalar_vector_comparison", vector_factory {}, [](auto& _) {

        using T = mettle::fixture_type_t< decltype(_) >;

        _.test( "equal", []( polysync::variant truth ) {
                polysync::node correct( "array", std::vector<T> { 17, 42, 37 } );
                expect( correct, equal_to(truth) );
                });

        _.test( "notequal", []( polysync::variant truth ) {
                polysync::node wrong( "array", std::vector<T> { 17, 42, 47 } );
                expect( wrong, not_equal_to(truth) );
                });

        _.test( "tooshort", []( polysync::variant truth ) {
                polysync::node tooshort( "array", std::vector<T> { 17, 42 } );
                expect( tooshort, not_equal_to(truth) );
                });

        _.test( "toolong", []( polysync::variant truth ) {
                polysync::node toolong( "array", std::vector<T> { 17, 42, 37, 56 } );
                expect( toolong, not_equal_to(truth) );
                });

        _.test( "wrong_type", []( polysync::variant truth ) {
                polysync::node toolong( "array", std::vector<float> { 17, 42, 37, 56 } );
                expect( toolong, not_equal_to(truth) );
                });
});

struct tree_factory {
    template <typename T>
    polysync::variant make() {
	    return polysync::tree( "type", {
			    { "start_time", std::uint16_t { 1 } },
			    { "scanner_count", std::uint32_t { 2 } }
			    });
    }
};

mettle::suite<void> tree( "tree_comparison", tree_factory {}, [](auto& _) {

        _.test( "equal", []( polysync::variant truth ) {
                polysync::tree equal = polysync::tree( "type", {
                                { "start_time", std::uint16_t { 1 } },
                                { "scanner_count", std::uint32_t { 2 } }
                                });

                expect( equal, equal_to(truth) );
                });

        _.test( "notequal", []( polysync::variant truth ) {
                polysync::tree notequal = polysync::tree( "type", {
                                { "start_time", std::uint16_t { 1 } },
                                { "scanner_count", std::uint32_t { 3 } }
                        });

                expect( notequal, not_equal_to(truth) );
                });

        _.test( "tooshort", []( polysync::variant truth ) {
                polysync::tree tooshort = polysync::tree( "type", {
                                { "start_time", std::uint16_t { 1 } },
                        });

                expect( tooshort, not_equal_to(truth) );
                });

        _.test( "toolong", []( polysync::variant truth ) {
                polysync::tree toolong = polysync::tree( "type", {
                                { "start_time", std::uint16_t { 1 } },
                                { "scanner_count", std::uint32_t { 2 } },
                                { "stop_time", std::uint16_t { 3 } },
                        });

                expect( toolong, not_equal_to(truth) );
                });

        _.test( "wrong_name", []( polysync::variant truth ) {
                polysync::tree equal = polysync::tree( "other_type", {
                                { "start_time", std::uint16_t { 1 } },
                                { "scanner_count", std::uint32_t { 2 } }
                                });

                expect( equal, not_equal_to(truth) );
                });

        _.test( "wrong_element_name", []( polysync::variant truth ) {
                polysync::tree equal = polysync::tree( "type", {
                                { "another_time", std::uint16_t { 1 } }, // wrong name
                                { "scanner_count", std::uint32_t { 2 } }
                                });

                expect( equal, not_equal_to(truth) );
                });

});

struct nested_tree_factory {
    template <typename T>
    polysync::variant make() {
	    return polysync::tree( "type", {
                        { "header", std::uint16_t { 1 } },
			    { "start_time", std::uint16_t { 1 } },
			    { "scanner_count", std::uint32_t { 2 } }
			    });
    }
};

mettle::suite<void> nested_tree( "nested_tree_comparison", nested_tree_factory {}, 
		[](auto& _) {

        _.test( "equal", []( polysync::variant truth ) {
                polysync::tree equal = polysync::tree( "type", {
                        { "header", std::uint16_t { 1 } },
                                { "start_time", std::uint16_t { 2 } },
                                { "scanner_count", std::uint32_t { 3 } }
                        });

                expect( equal, equal_to(truth) );
                });

        _.test( "notequal", []( polysync::variant truth ) {
                polysync::tree notequal = polysync::tree( "type", {
                        { "header", std::uint16_t { 1 } },
                                { "start_time", std::uint16_t { 2 } },
                                { "scanner_count", std::uint32_t { 4 } }
                        });

                expect( notequal, not_equal_to(truth) );
                });

        _.test( "tooshort", []( polysync::variant truth ) {
                polysync::tree tooshort = polysync::tree( "type", {
                        { "header", std::uint16_t { 1 } },
                                { "start_time", std::uint16_t { 2 } },
                        });

                expect( tooshort, not_equal_to(truth) );
                });

        _.test( "toolong", []( polysync::variant truth ) {
                polysync::tree toolong = polysync::tree( "type", {
                        { "header", std::uint16_t { 1 } },
                                { "start_time", std::uint16_t { 2 } },
                                { "scanner_count", std::uint32_t { 3 } },
                                { "stop_time", std::uint16_t { 4 } },
                        });

                expect( toolong, not_equal_to(truth) );
                });
});


