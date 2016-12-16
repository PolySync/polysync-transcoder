#pragma once

#include <polysync/print.hpp>
#include <polysync/tree.hpp>

namespace polysync {

inline std::ostream& operator<<( std::ostream& os, const variant& n ) {
    eggs::variants::apply( [ &os ]( auto a ) { os << a; }, n );
    return os;
}

inline std::ostream& operator<<( std::ostream& os, const node& n ) {

    if ( n.target_type() == typeid(tree) ) {
	return os << n.name << ": " << *n.target<tree>();
    }

    // special case std::uint8_t so it does not print like a char.
    if ( n.target_type() == typeid(std::uint8_t) ) {
        return os << static_cast<std::uint16_t>( *n.target<std::uint8_t>() );
    }

    if ( n.format ) {
        return os << n.format(n);
    }
		
    eggs::variants::apply( [ &os ]( auto a ) { os << a; }, n );
    return os;
}

} // namespace polysync
