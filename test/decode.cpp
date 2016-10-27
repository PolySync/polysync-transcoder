#include <mettle.hpp>

#include <polysync/plog/decoder.hpp>
#include <polysync/plog/encoder.hpp>

using namespace mettle;
namespace plog = polysync::plog;
namespace hana = boost::hana;

constexpr auto integers = hana::tuple_t<
    std::int8_t, std::int16_t, std::int32_t, std::int64_t,
    std::uint8_t, std::uint16_t, std::uint32_t, std::uint64_t
    >;

constexpr auto reals = hana::tuple_t<float, double>;

auto bigendians = hana::transform(integers, [](auto t) { 
        using namespace boost::endian;
        using T = typename decltype(t)::type;
        return hana::type_c<endian_arithmetic<order::big, T, 8*sizeof(T)>>; 
        });

// Define metafunction to compute parameterized mettle::suite from a hana typelist.
template <typename TypeList>
using type_suite = typename decltype(
        hana::unpack(std::declval<TypeList>(), hana::template_<mettle::suite>)
        )::type;

struct number_factory {
    template <typename T>
    plog::variant make() { return static_cast<T>(42); }
};

type_suite<decltype(hana::concat(integers, reals))> 

decode("number", number_factory {}, [](auto& _) {

    _.test("decode", [](auto value) {
            using T = mettle::fixture_type_t<decltype(_)>;
            std::stringstream stream;
            stream.write((char *)&value, sizeof(value));

            expect(plog::decoder(stream).decode<T>(), equal_to(42));
            });

    _.test("encode", [](auto value) {
            using T = mettle::fixture_type_t<decltype(_)>;
            std::stringstream stream;
            plog::encoder encode(stream);
            encode.encode(value);

            T result;
            stream.read((char *)&result, sizeof(T));
            expect(result, equal_to(42));
            });
    });
