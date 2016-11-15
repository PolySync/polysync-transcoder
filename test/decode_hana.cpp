#include <mettle.hpp>

#include <boost/hana/equal.hpp>

#include <polysync/plog/decoder.hpp>
#include <polysync/plog/encoder.hpp>
#include "types.hpp"

using namespace mettle;
namespace plog = polysync::plog;
namespace hana = boost::hana;

// Instantiate the static console format; this is used inside of mettle to
// print failure messages through operator<<'s defined in io.hpp.
namespace polysync { namespace console { style format = color(); }}

struct number_factory {
    template <typename T>
    polysync::variant make() { return static_cast<T>(42); }
};

struct hana_factory {
    template <typename T> T make();
};

template <>
plog::log_module hana_factory::make<plog::log_module>() {
    return { 11, 21, 31, 41, 51, "log" }; 
}

template <>
plog::type_support hana_factory::make<plog::type_support>() {
    return { 21, "type" }; 
}

template <>
plog::log_header hana_factory::make<plog::log_header>() {
    return { 1, 2, 3, 4, 5, {}, {} }; 
}

template <>
plog::msg_header hana_factory::make<plog::msg_header>() {
    return { 10, 20, 30 };
}

template <>
plog::log_record hana_factory::make<plog::log_record>() {
    return { 110, 120, 130, 250, "bigblob" };
}
        

type_suite<decltype(scalars)> 
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
            plog::encoder(stream).encode(value);

            T result;
            stream.read((char *)&result, sizeof(T));
            expect(result, equal_to(42));
            });

    _.test("transcode", [](auto value) {
            using T = mettle::fixture_type_t<decltype(_)>;

            std::stringstream stream;
            plog::encoder encode(stream);
            plog::decoder decode(stream);
            encode.encode(value);
            expect(decode.decode<T>(), equal_to(42));
            });
    });

mettle::suite<plog::log_header, plog::msg_header, plog::log_module, plog::type_support, plog::log_record> 
structures("structures", hana_factory {}, [](auto& _) {

        _.test("transcode", [](auto cls) {
                using T = mettle::fixture_type_t<decltype(_)>;

                std::stringstream stream;
                plog::encoder write(stream);
                plog::decoder read(stream);

                write.encode(cls);
                expect(read.decode<T>(), hana_equal(cls));

                });

        });
