#pragma once

#include <polysync/console.hpp>
#include <polysync/tree.hpp>
#include <polysync/print_vector.hpp>

namespace polysync 
{

inline std::ostream& operator<<( std::ostream& os, const variant& n );

inline std::ostream& operator<<( std::ostream& os, const tree& value ) 
{
    os << value.type << ": { ";
    for ( const node& n: *value ) 
    {
        os << format->fieldname(n.name + ": ") << static_cast<variant>(n) << ", ";
    }
    os.seekp( -2, std::ios_base::end ); // remove that last comma
    return os << " }";
}

inline std::ostream& operator<<( std::ostream& os, const variant& n ) 
{
    // for 8 bit integers, print them as 16 bits so they don't print like chars
    if ( n.target_type() == typeid(std::int8_t) ) 
    {
        return os << static_cast<std::int16_t>(*n.target<std::int8_t>());
    }

    if ( n.target_type() == typeid(std::uint8_t) ) 
    {
        return os << static_cast<std::uint16_t>(*n.target<std::uint8_t>());
    }

    eggs::variants::apply( [ &os ]( auto a ) { os << a; }, n );
    return os;
}

inline std::ostream& operator<<( std::ostream& os, const node& n ) {
    if ( n.format ) 
    {
        return os << n.format(n);
    }

    // trees have a useful type name, so print that
    if ( n.target_type() == typeid(tree) ) 
    {
	os << n.name << ": ";
    }

    return os << static_cast<variant>(n);

}

} // namespace polysync


