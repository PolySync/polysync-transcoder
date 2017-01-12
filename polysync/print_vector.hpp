#pragma once

#include <polysync/console.hpp>

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

