#pragma once

#include <sstream>
#include <string>
#include <vector>
#include <algorithm>

#include <boost/hana.hpp>

#include <polysync/console.hpp>
#include <polysync/print.hpp>

// Implement pretty printers for hana::pair<>

namespace boost { namespace hana {

using polysync::format;

template <typename T, typename U>
std::ostream& operator<<( std::ostream& os, const pair<T, U>& p ) {
    return os << format->fieldname( std::string(hana::to<char const*>(hana::first(p))) ) 
              << ": " << hana::second(p);
}

template <typename U>
std::ostream& operator<<( std::ostream& os, const pair<std::string, U>& p ) {
    return os << format->fieldname( hana::first(p) + ": " ) << hana::second(p);
}

// Specialize the std::uint8_t case so it does not print an ASCII character, but rather an int
template <typename T>
std::ostream& operator<<( std::ostream& os, const pair<T, std::uint8_t>& p ) {
    return os << format->fieldname( std::string( hana::to<char const*>( hana::first(p) ) ) + ": " )
        << std::to_string( (std::uint16_t)hana::second(p) ) << ";";
}

}} // boost::hana

