#pragma once

#include <boost/hana.hpp>

#include <polysync/tree.hpp>

namespace polysync {

namespace hana = boost::hana;

// Convert a hana structure into a vector of dynamic nodes.
template <typename S, typename = std::enable_if_t<hana::Struct<S>::value> >
node from_hana( const S& s, const std::string& type ) {
    tree tr(type);
    hana::for_each( s, [tr](auto pair) {
            std::string name = hana::to<char const*>( hana::first(pair) );
            tr->emplace_back( name, hana::second(pair) );
            });

    return node( type, tr );
}

} // namespace polysync


