#pragma once

#include <boost/hana.hpp>
#include <fstream>

namespace polysync { namespace plog {

namespace hana = boost::hana;

template <typename Number, class Enable = void>
struct size {
    static std::streamoff value() { return sizeof(Number); }
};

template <typename Struct>
struct size<Struct, typename std::enable_if<hana::Foldable<Struct>::value>::type> {
    static std::streamoff value() {
        return hana::fold(hana::members(Struct()), 0, [](std::streamoff s, auto field) { 
                return s + size<decltype(field)>::value(); 
                });
    }
};


}} // namespace polysync:plog
