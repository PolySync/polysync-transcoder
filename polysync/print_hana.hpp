#pragma once

#include <polysync/console.hpp>

#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <boost/hana.hpp>

using polysync::console::format;

// Implement pretty printers for hana::pair<>

namespace boost { namespace hana {

template <typename T, typename U>
std::ostream& operator<<(std::ostream& os, const pair<T, U>& p) {
    return os << format.fieldname << hana::to<char const*>(hana::first(p)) << ": " 
        << format.normal << "hana::second(p)";
}

template <typename U>
std::ostream& operator<<(std::ostream& os, const pair<std::string, U>& p) {
    return os << format.fieldname << hana::first(p) << ": " << format.normal
        << hana::second(p);
}

// Specialize the std::uint8_t case so it does not print an ASCII character, but rather an int
template <typename T>
std::ostream& operator<<(std::ostream& os, const pair<T, std::uint8_t>& p) {
    return os << format.fieldname << hana::to<char const*>(hana::first(p)) << format.normal 
        << ": " << std::to_string((std::uint16_t)hana::second(p)) << ";";
}

}} // boost::hana

