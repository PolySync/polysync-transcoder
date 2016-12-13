#pragma once

#include <boost/multiprecision/cpp_int.hpp>
#include <boost/endian/arithmetic.hpp>

#include <polysync/exception.hpp>

namespace polysync {

// Match all integer types to swap the bytes; specialize for non-integers below.
template <typename T>
typename std::enable_if<std::is_integral<T>::value, T>::type
byteswap(const T& value) {
    boost::endian::endian_arithmetic<boost::endian::order::big, T, 8*sizeof(T)> swap(value);
    return *(new ((void *)swap.data()) T);
}

float byteswap(float value) {
    // Placement new native float bytes into an int
    std::uint32_t bytes = *(new ((void *)&value) std::uint32_t);
    boost::endian::big_uint32_t swap(bytes);
    // Placement new back into a byte-swapped float
    return *(new ((void *)swap.data()) float);
}

double byteswap(double value) {
    // Placement new native float bytes into an int
    std::uint64_t bytes = *(new ((void *)&value) std::uint64_t);
    boost::endian::big_uint64_t swap(bytes);
    // Placement new back into a byte-swapped double
    return *(new ((void *)swap.data()) double);
}

// Non-arithmetic types should not ever be marked bigendian.
template <typename T>
typename std::enable_if<!std::is_arithmetic<T>::value, T>::type
byteswap(const T&) {
    throw polysync::error("non-arithmetic type cannot be byteswapped");
}

}

