#include <mettle.hpp>

#include <boost/hana/equal.hpp>

#include <polysync/size.hpp>
#include <polysync/byteswap.hpp>
#include <polysync/plog/decoder.hpp>
#include <polysync/plog/encoder.hpp>
#include <polysync/print_hana.hpp>
#include <polysync/print_tree.hpp>
#include "types.hpp"

using namespace mettle;
namespace plog = polysync::plog;
namespace hana = boost::hana;
namespace mp = boost::multiprecision;
namespace ps = polysync;

// Instantiate the static console format; this is used inside of mettle to
// print failure messages through operator<<'s defined in io.hpp.

struct number_factory {
    template <typename T>
    polysync::variant make() { return T { 42 }; }
};

struct hana_factory {
    template <typename T> T make();
};

template <>
plog::log_module hana_factory::make<plog::log_module>() {
    return { 11, 21, 31, 41, mp::cpp_int("0xF0E1D2C3B4A50697" "88796A5B4C3D2E1"), "log" };
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
    return { 110, 120, 130, 250 };
}


type_suite<decltype(scalars)>
decode("number", number_factory {}, [](auto& _) {

    _.test("decode", [](auto value) {
            using T = mettle::fixture_type_t<decltype(_)>;
            std::stringstream stream;
            stream.write((char *)&value, polysync::size<T>::value());

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

mettle::suite<>
hash("hash_type", [](auto& _) {
    namespace multiprecision = boost::multiprecision;

    // multiprecision::cpp_int types are failing when there are leading zero
    // values, I think because the library is only streaming out the nonzero
    // values.  I would rather use a fixed multiprecision type here so it
    // always streams the exact number of bytes zero or not, but export_bits
    // has a bug with the 128 bit type, that has been fixed in boost upstream,
    // but the fix is not available in boost 1.62.
    //
    // For now, this is not a big deal because the PolySync hash_type is
    // probably never going to have leading zeros, and that is so far the only
    // place the multiprecision type is used.

    _.test("decode", []() {
            std::stringstream stream;
            multiprecision::cpp_int src("0xDEADBEEF01234567" "F0E1D2C3B4A59687");
            multiprecision::export_bits(src, std::ostream_iterator<std::uint8_t>(stream), 8);
            expect(plog::decoder(stream).decode<multiprecision::cpp_int>(), equal_to(src));
            });

    _.test("encode", []() {
            multiprecision::cpp_int value { 42 };
            std::stringstream stream;
            plog::encoder(stream).encode(value);

            polysync::bytes result(16);
            stream.read((char *)result.data(), result.size());
            polysync::bytes truth = { 42, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
            expect(result, equal_to(truth));
            });

    _.test("transcode", []() {
            multiprecision::cpp_int value("0xDEADBEEF01234567" "F0E1D2C3B4A59687");

            std::stringstream stream;
            plog::encoder encode(stream);
            plog::decoder decode(stream);

            encode.encode(value);
            expect(decode.decode<multiprecision::cpp_int>(), equal_to(value));
            });
        });

mettle::suite<plog::log_header, plog::msg_header, plog::log_module, plog::type_support, plog::log_record>
structures("structures", hana_factory {}, [](auto& _) {

        _.test("transcode", [](auto value) {
                using T = mettle::fixture_type_t<decltype(_)>;

                std::stringstream stream;
                plog::encoder encode(stream);
                plog::decoder decode(stream);

                encode.encode(value);
                expect(decode.decode<T>(), hana_equal(value));

                });

        });

mettle::suite<> structures2("structures2", [](auto& _) {
        _.subsuite("log_module", [](auto&_) {
                _.test("encode", []() {
                        plog::log_module record { 1, 2, 3, 4, 0, "name" };
                        record.build_hash = 0xFEDCBA9876543210;
                        record.build_hash <<= 64;
                        record.build_hash |= 0xF1E2D3C4B5A69788;
                        std::stringstream stream;
                        plog::encoder write(stream);
                        write(record);

                        std::string truth = "01" "02" "0300" "04000000"
                        "FEDCBA9876543210F1E2D3C4B5A69788"
                        "0400" "6E616D65";

                        mp::cpp_int blob;
                        std::string bin = stream.str();
                        mp::import_bits(blob, bin.begin(), bin.end(), 8);

                        std::stringstream hex;
                        hex << std::setfill('0') << std::setw(2*bin.size()) << std::hex << blob;
                        expect(hex.str(), equal_to(truth));
                        });

                _.test("decode", []() {
                        plog::log_module truth { 1, 2, 3, 4, 5, "name" };

                        std::string hex = "01" "02" "0300" "04000000"
                        "00000000000000000000000000000005"
                        "04006E616D65" // name_type is uint16 + 4 bytes "name"
                        ;

                        // Create a binary blob from the hex description
                        std::string blob;
                        mp::export_bits(mp::cpp_int("0x" + hex), std::back_inserter(blob), 8);

                        std::stringstream stream(blob);
                        plog::decoder decode(stream);
                        plog::log_module record;
                        decode.decode(record);

                        expect(record, hana_equal(truth));
                        });
                });
        });
