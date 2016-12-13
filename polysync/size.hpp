#pragma once

#include <boost/hana.hpp>
#include <boost/multiprecision/cpp_int.hpp>

namespace polysync {

// Define some metaprograms to compute the sizes of types.
template <typename Number, class Enable = void>
struct size {
    static std::streamoff value() { return sizeof(Number); }
};

template<>
struct size<boost::multiprecision::cpp_int> {
    static std::streamoff value() { return 16; }
};

template <typename S>
struct size<S, typename std::enable_if<boost::hana::Struct<S>::value>::type> {
    static std::streamoff value() {
        return boost::hana::fold(boost::hana::members(S()), 0, [](std::streamoff s, auto field) {
                return s + size<decltype(field)>::value();
                });
    }
};

}

