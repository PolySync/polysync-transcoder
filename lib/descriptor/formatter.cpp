#include <bitset>
#include <iostream>

#include <boost/numeric/conversion/cast.hpp>

#include <eggs/variant.hpp>

#include <polysync/descriptor/formatter.hpp>
#include <polysync/print_tree.hpp>

namespace polysync { namespace descriptor {

// Print chars as integers
std::ostream& operator<<( std::ostream& os, std::uint8_t value )
{
    return os << (std::uint16_t)value;
}

std::ostream& operator<<( std::ostream& os, std::int8_t value )
{
    return os << (std::int16_t)value;
}

// print_binary is incomplete because I never figured out how to pass in the
// number of bits to print.
struct print_binary
{
    template <typename T>
    typename std::enable_if_t< std::is_integral<T>::value, std::string>
    operator()( const T& value ) const
    {
        std::bitset<8> bits ( value );

        std::stringstream ss;
        ss << bits;
        return ss.str();
    }

    template <typename T>
    typename std::enable_if_t< !std::is_integral<T>::value, std::string>
    operator()( const T& ) const
    {
        throw polysync::error( "type cannot be formatted as binary" );
    }

};

std::map< std::string, decltype( Field::format ) > formatFunction
{
    { "hex", []( const Variant& value )
        {
            // The hex formatter just uses the default operator<<(), but kicks on std::hex first.
            std::stringstream os;
            eggs::variants::apply( [ &os ]( auto v ) { os << std::hex << "0x" << v << std::dec; }, value );
            return os.str();
        }
    },
    { "decimal_and_hex", []( const Variant& value )
        {
            std::stringstream os;
            eggs::variants::apply( [ &os ]( auto v ) {
                    os << std::dec << v << " (" << std::hex << "0x" << v << ")" << std::dec;
                    }, value );
            return os.str();
        }
    },
    { "binary", []( const Variant& value )
        {
            return eggs::variants::apply( print_binary(), value );
        }
    }
};

}} // namespace polysync::descriptor
