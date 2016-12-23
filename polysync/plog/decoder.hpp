#pragma once

#include <fstream>

#include <boost/endian/arithmetic.hpp>

#include <polysync/tree.hpp>
#include <polysync/descriptor.hpp>
#include <polysync/logging.hpp>
#include <polysync/plog/iterator.hpp>

namespace polysync { namespace plog {

class decoder {
public:

    decoder( std::istream& st );

    // Construct STL compatible iterators
    iterator begin();
    iterator end();

    // Decode an entire record and return a parse tree.
    variant deep( const ps_log_record& );

    variant operator()( const descriptor::Type& );

public:
    // The decode() members are public only because the unit tests call them.

    // Dynamic compound type from descriptor
    variant decode( const descriptor::Type& );

    // Dynamic compound type, but look up the description first by name
    variant decode( const std::string& type );

    // Factory functions of any supported type

    template <typename T>
    T decode( std::streamoff pos );

    template <typename T>
    T decode();

    // Native arithmetic types (floats and integers)
    template <typename Number>
    typename std::enable_if_t< std::is_arithmetic<Number>::value >
    decode( Number& );

    // Integers longer than 64 bits (like ps_hash_type)
    void decode( boost::multiprecision::cpp_int& );

    // Endian swapped types
    template < typename T, size_t N >
    void decode( boost::endian::endian_arithmetic< boost::endian::order::big, T, N >& );

    // Hana exposed structures
    template <typename S, class = typename std::enable_if_t< boost::hana::Struct<S>::value> >
    void decode( S& );

    // PolySync sequences (integer size, followed by that many bytes)
    template <typename LenType, typename T>
    void decode( sequence<LenType, T>& );

    // PolySync name type is specialized as std::string
    void decode( ps_name_type& name );

    // Raw, uninterpreted bytes used when type description is unavailable
    void decode( bytes& raw );

    // Decode any type, but seek to a known position first
    template <typename T>
    void decode( T&, std::streamoff );

protected:

    using parser = std::function<variant (decoder&)>;
    static std::map<std::string, parser> parse_map;

    logging::logger log { "plog::decoder" };

    friend struct branch_builder;
    friend struct iterator;

    std::istream& stream;

    std::streamoff record_endpos; // the last byte of the current record
};

template <typename Number>
typename std::enable_if_t< std::is_arithmetic<Number>::value >
decoder::decode( Number& value )
{
    stream.read( (char *)(&value), sizeof(Number) );
}

// Endian swapped types
template <typename T, size_t N>
void decoder::decode( boost::endian::endian_arithmetic<boost::endian::order::big, T, N>& value )
{
    stream.read( (char *)value.data(), sizeof(T) );
}

// Specialize decode() for hana wrapped structures.  This works out padding
// in the structure definition where a straight memcpy would fail, and also
// recurses into nested structures.  Hana has a function hana::members()
// which would make this simpler, but sadly members() cannot return
// non-const references which we need here (we are setting the value).
template <typename S, class>
void decoder::decode( S& record )
{
    namespace hana = boost::hana;
    hana::for_each( hana::keys(record), [this, &record](auto&& key) mutable {
            this->decode( hana::at_key(record, key) );
            });
}

// Sequences have a LenType number first specifying how many T's follow.  T
// might be a flat (arithmetic) type or a nested structure.  Either way,
// iterate the fields and serialize each one using the specialized decode().
template <typename LenType, typename T>
void decoder::decode( sequence<LenType, T>& seq )
{
    LenType len;
    stream.read( reinterpret_cast< char* >( &len ), sizeof( len ) );
    seq.resize(len);
    std::for_each(seq.begin(), seq.end(), [this]( auto& val ) mutable
            { this->decode(val); });
}

  // Factory function of any supported type, for convenience
template <typename T>
T decoder::decode()
{
    try
    {
        T value;
        decode(value);
        return value;
    }
    catch ( std::ios_base::failure )
    {
        throw polysync::error("read error")
            << exception::module("plog::decoder");
    }
}

// Deserialize a structure from a known offset in the file.  This is the method
// used when dereferencing the iterator, because the iterator knows the offset
// of a particular record.  We have both a factory function version and set by
// reference.
template <typename T>
void decoder::decode( T& value, std::streamoff pos )
{
    try
    {
        stream.seekg( pos );
        decode<T>( value );
    }
    catch ( std::ifstream::failure )
    {
        throw polysync::error( "short read" );
    }
}

template <typename T>
T decoder::decode( std::streamoff pos )
{
    stream.seekg(pos);
    return decode<T>();
}


}} // namespace polysync::plog
