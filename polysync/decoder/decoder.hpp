#pragma once

#include <fstream>

#include <boost/endian/arithmetic.hpp>

#include <polysync/tree.hpp>
#include <polysync/hana.hpp>
#include <polysync/descriptor.hpp>
#include <polysync/detector.hpp>
#include <polysync/logging.hpp>
#include <polysync/compound_types.hpp>
#include <polysync/decoder/iterator.hpp>

namespace polysync {

class Decoder
{
public:

    Decoder( std::istream& st );

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
    template <typename LenType>
    void decode( sequence<LenType, std::uint8_t>& name );

    // Raw, uninterpreted bytes used when type description is unavailable
    void decode( bytes& raw );

    // Decode any type, but seek to a known position first
    template <typename T>
    void decode( T&, std::streamoff );

protected:

    using Parser = std::function<variant ( Decoder& )>;
    static std::map<std::string, Parser> parseMap;

    logging::logger log { "decoder" };

    friend struct branch_builder;

    std::istream& stream;

    std::streamoff record_endpos; // the last byte of the current record
};

template <typename Number>
typename std::enable_if_t< std::is_arithmetic<Number>::value >
Decoder::decode( Number& value )
{
    stream.read( (char *)(&value), sizeof(Number) );
}

// Endian swapped types
template <typename T, size_t N>
void Decoder::decode( boost::endian::endian_arithmetic<boost::endian::order::big, T, N>& value )
{
    stream.read( (char *)value.data(), sizeof(T) );
}

// Specialize decode() for hana wrapped structures.  This works out padding
// in the structure definition where a straight memcpy would fail, and also
// recurses into nested structures.  Hana has a function hana::members()
// which would make this simpler, but sadly members() cannot return
// non-const references which we need here (we are setting the value).
template <typename S, class>
void Decoder::decode( S& record )
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
void Decoder::decode( sequence<LenType, T>& seq )
{
    LenType len;
    stream.read( reinterpret_cast< char* >( &len ), sizeof( len ) );
    seq.resize(len);
    std::for_each(seq.begin(), seq.end(), [this]( auto& val ) mutable
            { this->decode(val); });
}

// Specialize name_type because the underlying std::string type needs special
// handling.  It resembles a Pascal string (length first, no trailing zero
// as a C string would have)
template <typename LenType>
void Decoder::decode( sequence<LenType, std::uint8_t>& name )
{
    std::uint16_t len;
    stream.read( (char*)( &len ), sizeof(len) );
    name.resize( len );
    stream.read( (char*)( name.data() ), len );
}


  // Factory function of any supported type, for convenience
template <typename T>
T Decoder::decode()
{
    try
    {
        T value;
        decode(value);
        return value;
    }
    catch ( std::ios_base::failure )
    {
        throw polysync::error("read error") << exception::module("decoder");
    }
}

// Deserialize a structure from a known offset in the file.  This is the method
// used when dereferencing the iterator, because the iterator knows the offset
// of a particular record.  We have both a factory function version and set by
// reference.
template <typename T>
void Decoder::decode( T& value, std::streamoff pos )
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
T Decoder::decode( std::streamoff pos )
{
    stream.seekg(pos);
    return decode<T>();
}

template <typename Header>
struct Sequencer : Decoder
{
    using Decoder::Decoder;

    // Construct STL compatible iterators
    Iterator<Header> begin()
    {
        return Iterator<Header>( this, stream.tellg() );
    }

    Iterator<Header> end()
    {
        // Find the last byte of the file
        std::streamoff current = stream.tellg();
        stream.seekg( 0, std::ios_base::end );
        std::streamoff end = stream.tellg();
        stream.seekg( current );

        return Iterator<Header> ( end );
    }

    // Kick off a decode with the header type. Continue reading the stream until it ends.
    template <typename T>
    variant deep( const Header& record )
    {
        variant result = from_hana(record, descriptor::terminalTypeMap.at(typeid(Header)).name);
        polysync::tree& tree = *result.target<polysync::tree>();

        record_endpos = stream.tellg() + static_cast<std::streamoff>(record.size);

        // Decode a sequence of static types, if given.  No detectors or
        // branching is possible with static types, but decoding is much faster.
        tree->emplace_back(from_hana( decode<T>(), descriptor::terminalTypeMap.at(typeid(T)).name ) );

        // Burn through the rest of the log record, decoding a sequence of
        // dynamic types, for which detectors and descriptors must exist.
        while ( stream.tellg() < record_endpos )
        {
            std::string type = detect( tree->back() );
            tree->emplace_back( type, decode(type) );
        }
        return std::move(result);
    }

    friend struct Iterator<Header>;
};

} // namespace polysync
