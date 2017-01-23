#pragma once

#include <boost/hana.hpp>

#include <polysync/tree.hpp>

namespace polysync {

namespace hana = boost::hana;

// Convert a hana structure into a vector of dynamic nodes.
template < typename S, typename = std::enable_if_t< hana::Struct<S>::value > >
Node from_hana( const S& s, const std::string& type ) {
    Tree tr(type);
    hana::for_each( s, [tr]( auto pair ) {
            std::string name = hana::to<char const*>( hana::first(pair) );
            tr->emplace_back( name, hana::second(pair) );
            });

    return Node( type, tr );
}

} // namespace polysync


