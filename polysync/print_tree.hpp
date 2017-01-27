#pragma once

#include <polysync/console.hpp>
#include <polysync/tree.hpp>
#include <polysync/print_vector.hpp>

namespace polysync
{

inline std::ostream& operator<<( std::ostream& os, const Variant& n );

inline std::ostream& operator<<( std::ostream& os, const Tree& value )
{
    os << value.type << ": { ";
    for ( const Node& n: *value )
    {
        os << format->fieldname(n.name + ": ") << static_cast<Variant>(n) << ", ";
    }
    os.seekp( -2, std::ios_base::end ); // remove that last comma
    return os << " }";
}

inline std::ostream& operator<<( std::ostream& os, const Variant& n )
{
    // for 8 bit integers, print them as 16 bits so they don't print like chars
    if ( n.target_type() == typeid(std::int8_t) )
    {
        return os << static_cast<std::int16_t>( *n.target<std::int8_t>() );
    }

    if ( n.target_type() == typeid(std::uint8_t) )
    {
        return os << static_cast<std::uint16_t>( *n.target<std::uint8_t>() );
    }

    eggs::variants::apply( [ &os ]( auto a ) { os << a; }, n );
    return os;
}

inline std::ostream& operator<<( std::ostream& os, const Node& n )
{
    if ( n.format )
    {
        return os << n.format(n);
    }

    // trees have a useful type name, so print that
    if ( n.target_type() == typeid(Tree) )
    {
	    os << n.name << ": ";
    }

    return os << static_cast<Variant>(n);
}

} // namespace polysync


