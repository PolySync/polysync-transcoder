#pragma once

#include <boost/endian/arithmetic.hpp>

#include <polysync/tree.hpp>
#include <polysync/description.hpp>
#include <polysync/plog/core.hpp>
#include <polysync/logging.hpp>
#include <fstream>

namespace polysync { namespace plog {

namespace hana = boost::hana;
using logging::severity;

// Indirecting and incrementing an iterator requires reading the plog file
// itself, because the offsets between records are always different, which we
// cannot do until plog::decoder is defined.  So implement these functions
// below, after decoder definition.  In the meantime, forward declare decoder
// so we can prototype the methods.
struct decoder;

// Define an STL compatible iterator to traverse plog files
struct iterator {

    iterator(decoder*, std::streamoff);

    decoder* stream;
    std::streamoff pos; // file offset from std::ios_base::beg, pointing to next record
    log_record header;

    bool operator!=(const iterator& other) const { return pos < other.pos; }
    bool operator==(const iterator& other) const { return pos == other.pos; }

    log_record operator*(); // read and return the payload
    iterator& operator++(); // skip to the next record
};

class decoder {
public:

    decoder(std::istream& st) : stream(st) {
        stream.exceptions(std::ifstream::failbit);

        // Find the last byte of the file
        stream.seekg(0, std::ios_base::end);
        endpos = stream.tellg();
        stream.seekg(0, std::ios_base::beg);
    }

    // Factory methods for STL compatible iterators
    iterator begin() { return iterator (this, stream.tellg()); }
    iterator end() { return iterator (nullptr, endpos); }

public:
    // Decode an entire record and return a parse tree.
    variant deep(const log_record&);
    variant operator()(const descriptor::type& type) { return decode(type); }

public:
    // Define a set of decode() templates, overloads, and specializations to pattern
    // match every possible struct, sequence, or terminal type, nested or not,
    // that is possibly found in a message.

    // For flat objects like arithmetic types, just straight copy memory as
    // blob to file. In particular, most types that are not hana structures are these.
    template <typename Number>
    typename std::enable_if_t<std::is_arithmetic<Number>::value>
    decode(Number& value) {
        stream.read((char *)(&value), sizeof(Number));
    }

    // Endian swapped types
    template <typename T, size_t N>
    void decode(boost::endian::endian_arithmetic<boost::endian::order::big, T, N>& value) {
        stream.read((char *)value.data(), sizeof(T));
    }

    void decode(hash_type& value) {
        bytes buf(16); // 16 should be a template param, but I cannot get it to match.
        stream.read((char *)buf.data(), buf.size());
        multiprecision::import_bits(value, buf.begin(), buf.end(), 8);
    }

    // Specialize decode() for hana wrapped structures.  This works out padding
    // in the structure definition where a straight memcpy would fail, and also
    // recurses into nested structures.  Hana has a function hana::members()
    // which would make this simpler, but sadly members() cannot return
    // non-const references which we need here (we are setting the value).
    template <typename S, class = typename std::enable_if_t<hana::Struct<S>::value>>
    void decode(S& record) {
        hana::for_each(hana::keys(record), [&](auto&& key) mutable {
                decode(hana::at_key(record, key));
                });
    }

    // Sequences have a LenType number first specifying how many T's follow.  T
    // might be a flat (arithmetic) type or a nested structure.  Either way,
    // iterate the fields and serialize each one using the specialized decode().
    template <typename LenType, typename T>
    void decode(sequence<LenType, T>& seq) {
        LenType len;
        stream.read(reinterpret_cast<char *>(&len), sizeof(len));
        seq.resize(len);
        std::for_each(seq.begin(), seq.end(), [=](auto&& val) mutable { decode(val); });
    }

    // Specialize name_type because the underlying std::string type needs special
    // handling.  It resembles a Pascal string (length first, no trailing zero
    // as a C string would have)
    void decode(name_type& name) {
        std::uint16_t len;
        stream.read((char *)(&len), sizeof(len));
        name.resize(len);
        stream.read((char *)(name.data()), len);
    }

    void decode(bytes& raw) {
        stream.read((char *)raw.data(), raw.size());
    }

    // Decode a dynamic type using the description name.
    variant decode(const std::string& type, bool bigendian = false);

    variant decode(const descriptor::type&);

    // Define a set of factory functions that know how to decode specific binary
    // types.  They keys are strings from the "type" field of the TOML descriptions.
    using parser = std::function<variant (decoder&)>;
    static std::map<std::string, parser> parse_map;

    // Factory function of any supported type, for convenience
    template <typename T>
    T decode() {
        T value;
        decode(value);
        return std::move(value);
    }

    // Deserialize a structure from a known offset in the file.  This is the
    // method used when dereferencing the iterator, because the iterator knows
    // the offset of a particular record.
    template <typename T>
    T decode(std::streamoff pos) {
        stream.seekg(pos);
        return decode<T>();
    }

    template <typename T>
    void decode(T& value, std::streamoff pos) {
        stream.seekg(pos);
        decode<T>(value);
    }


    // Expose the decode() members as operator(), for convenience.
    template <typename... Args>
    void operator()(Args... args) { decode(std::forward<Args>(args)...); }

    logging::logger log { "decoder" };

protected:

    friend struct branch_builder;
    friend struct iterator;

    std::istream& stream;

    std::streamoff endpos; // the last byte of the file
    std::streamoff record_endpos; // the last byte of the current record
};

inline iterator::iterator( decoder* s, std::streamoff pos ) : stream( s ), pos( pos ) {
    if ( stream ) {
        stream->decode( header, pos );
    }
}

inline log_record iterator::operator*() {
    return header;
}

inline iterator& iterator::operator++() {

    // Advance the iterator's position to the beginning of the next record.
    pos += descriptor::size<log_record>::value() + header.size;
    if ( pos < stream->endpos ) {
        stream->decode( header, pos );
    }
    return *this;
}

}} // namespace polysync::plog
