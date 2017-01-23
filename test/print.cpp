#include <mettle.hpp>

#include <polysync/tree.hpp>
#include <polysync/print_tree.hpp>
#include <polysync/console.hpp>

using namespace mettle;

mettle::suite<
    std::int8_t, std::int16_t, std::int32_t, std::int64_t,
    std::uint8_t, std::uint16_t, std::uint32_t, std::uint64_t,
    boost::multiprecision::cpp_int >
scalar( "print", mettle::type_only, [](auto& _) {

	using T = mettle::fixture_type_t< decltype(_) >;
	polysync::format = std::make_shared<polysync::formatter::plain>();

	_.test( "scalar", []() {
			std::stringstream os;
			polysync::Variant value = T { 42 };
			os << value;
			expect( os.str(), equal_to("42") );
			});

	_.test( "vector", []() {
			std::stringstream os;
			polysync::Variant value = std::vector<T> { 0x37, 0x42, 0x17 };
			os << value;
			expect( os.str(), equal_to("[ 37 42 17 ] (3 elements)") );
			});

	_.test( "tree", []() {
			std::stringstream os;
			polysync::Variant value = polysync::Tree ( "mytype", {
				{ "value1", T { 42 } },
				{ "value2", T { 17 } }
				});
			os << value;
			expect( os.str(), equal_to("mytype: { value1: 42, value2: 17 }") );
			});
        });



