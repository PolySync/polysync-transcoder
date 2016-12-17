#pragma once

#include <polysync/console.hpp>
#include <polysync/tree.hpp>

namespace polysync {
    inline std::ostream& operator<<( std::ostream& os, const variant& n );
}

namespace std {

template <typename T>
std::ostream& operator<<( std::ostream& os, const std::vector<T>& record ) {
    const int psize = 12;
    os << "[ " << std::hex;
    std::for_each( record.begin(), std::min(record.begin() + psize, record.end() ),
            [&os]( T value ) mutable { os << value << " "; });
    if ( record.size() > 2*psize )
        os << "... ";
    if ( record.size() > psize )
        std::for_each( std::max( record.begin() + psize, record.end() - psize), record.end(),
                       [ &os ]( T value ) mutable { os << value << " "; });
    os << "]" << std::dec << " (" << record.size() << " elements)";
    return os;
}

// overload std::vector<std::uint8_t> so it prints as integer not char
inline std::ostream& operator<<( std::ostream& os, const std::vector<std::uint8_t>& record ) {
    const int psize = 12;
    os << "[ " << std::hex;
    std::for_each( record.begin(), std::min(record.begin() + psize, record.end() ),
            [&os]( std::uint16_t value ) mutable { os << (value & 0xFF) << " "; });
    if ( record.size() > 2*psize )
        os << "... ";
    if ( record.size() > psize )
        std::for_each( std::max( record.begin() + psize, record.end() - psize), record.end(),
                       [ &os ]( std::uint16_t value ) mutable { os << (value & 0xFF) << " "; });
    os << "]" << std::dec << " (" << record.size() << " elements)";
    return os;
}

// overload std::vector<std::int8_t> so it prints as integer not char
inline std::ostream& operator<<( std::ostream& os, const std::vector<std::int8_t>& record ) {
    const int psize = 12;
    os << "[ " << std::hex;
    std::for_each( record.begin(), std::min(record.begin() + psize, record.end() ),
            [&os]( std::int16_t value ) mutable { os << (value & 0xFF) << " "; });
    if ( record.size() > 2*psize )
        os << "... ";
    if ( record.size() > psize )
        std::for_each( std::max( record.begin() + psize, record.end() - psize), record.end(),
                       [ &os ]( std::int16_t value ) mutable { os << (value & 0xFF) << " "; });
    os << "]" << std::dec << " (" << record.size() << " elements)";
    return os;
}

} // namespace std
namespace polysync {

inline std::ostream& operator<<( std::ostream& os, const tree& value ) {
    // eggs::variants::apply( [ &os ]( auto a ) { os << a; }, n );
    os << value.type << ": { ";
    for ( const node& n: *value ) {
        os << format->fieldname(n.name + ": ") << static_cast<variant>(n) << ", ";
    }
    os.seekp( -2, std::ios_base::end ); // remove that last comma
    return os << " }";
}

inline std::ostream& operator<<( std::ostream& os, const variant& n ) {
 
    // for 8 bit integers, print them as 16 bits so they don't print like chars
    if ( n.target_type() == typeid(std::int8_t) ) {
        return os << static_cast<std::int16_t>(*n.target<std::int8_t>());
    }

    if ( n.target_type() == typeid(std::uint8_t) ) {
        return os << static_cast<std::uint16_t>(*n.target<std::uint8_t>());
    }

    eggs::variants::apply( [ &os ]( auto a ) { os << a; }, n );
    return os;
}

inline std::ostream& operator<<( std::ostream& os, const node& n ) {
    if ( n.format ) {
        return os << n.format(n);
    }

    // trees have a useful type name, so print that
    if ( n.target_type() == typeid(tree) ) {
	os << n.name << ": ";
    }

    return os << static_cast<variant>(n);

}

} // namespace polysync


