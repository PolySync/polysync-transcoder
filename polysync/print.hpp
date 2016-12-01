#pragma once

#include <vector>
#include <iostream>

namespace std {

template <typename T>
std::ostream& operator<<(std::ostream& os, const std::vector<T>& record) {
    const int psize = 12;
    os << "[ " << std::hex;
    std::for_each(record.begin(), std::min(record.begin() + psize, record.end()), 
            [&os](auto& field) mutable { os << field << " "; });
    if (record.size() > 2*psize)
        os << "... ";
    if (record.size() > psize)
        std::for_each(std::max(record.begin() + psize, record.end() - psize), record.end(), [&os](auto& field) mutable { os << field << " "; });

    os << "]" << std::dec << " (" << record.size() << " elements)";
    return os;
}

// Specialize std::vector<std::uint8_t> so it does not print characters, but numbers.
inline std::ostream& operator<<(std::ostream& os, const std::vector<std::uint8_t>& record) {
    const int psize = 12;
    os << "[ " << std::hex;
    std::for_each(record.begin(), std::min(record.begin() + psize, record.end()), 
            [&os](auto& field) mutable { os << ((std::uint16_t)(field & 0xFF)) << " "; });
    if (record.size() > 2*psize)
        os << "... ";
    if (record.size() > psize)
        std::for_each(std::max(record.begin() + psize, record.end() - psize), record.end(), 
                [&os](auto& field) mutable { os << ((std::uint16_t)field & 0xFF) << " "; });

    os << "]" << std::dec << " (" << record.size() << " elements)";
    return os;
}


}

