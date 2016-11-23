#pragma once

#include <polysync/tree.hpp>
#include <polysync/description.hpp>
#include <polysync/plog/core.hpp>
#include <polysync/logging.hpp>
#include <fstream>

namespace polysync { namespace plog {

namespace hana = boost::hana;

// Indirecting and incrementing an iterator requires reading the plog file
// itself, because the offsets between records are always different, which we
// cannot do until plog::decoder is defined.  So implement these functions
// below, after decoder definition.  In the meantime, forward declare decoder
// so we can prototype the methods.
struct decoder;

// Define an STL compatible iterator to traverse plog files
struct iterator {

    decoder* stream;
    std::streamoff pos; // file offset from std::ios_base::beg, pointing to next record
    std::streamoff end; // filters need to know where the end is
    std::function<bool (iterator)> filter;

    bool operator!=(const iterator& other) const { return pos != other.pos; }
    bool operator==(const iterator& other) const { return pos == other.pos; }

    log_record operator*(); // read and return the payload
    log_record operator->(); // read and return the payload
    iterator& operator++(); // skip to the next record
};

class decoder {
public:

    decoder(std::istream& st) : stream(st) {
        stream.seekg(0, std::ios_base::end);
        endpos = stream.tellg();
        stream.seekg(0, std::ios_base::beg);
    }

    // Factory methods for STL compatible iterators

    iterator begin(std::function<bool (iterator)> filt) { 
        return iterator { this, stream.tellg(), endpos, filt }; 
    }

    iterator end() { return iterator { this, endpos, endpos }; }

public:
    // Decode an entire record and return a parse tree.
    node operator()(const log_record&);
    node operator()(const descriptor::type& type) { return decode(type); }

public:
    // Define a set of decode() templates, overloads, and specializations to pattern
    // match every possible struct, sequence, or terminal type, nested or not,
    // that is possibly found in a message.

    // For flat objects like arithmetic types, just straight copy memory as
    // blob to file. In particular, most types that are not hana structures are these.
    template <typename Number>
    typename std::enable_if_t<!hana::Foldable<Number>::value>
    decode(Number& value) {
        stream.read((char *)(&value), sizeof(Number)); 
    }
    
    // Specialize decode() for hana wrapped structures.  This works out padding
    // in the structure definition where a straight memcpy would fail, and also
    // recurses into nested structures.  Hana has a function hana::members()
    // which would make this simpler, but sadly members() cannot return
    // non-const references which we need here (we are setting the value).
    template <typename Record, 
             class = typename std::enable_if_t<hana::Foldable<Record>::value>>
    void decode(Record& record) {
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

    // Specialize log_record because the binary blob needs special handling
    void decode(log_record& record) {
        decode<log_record>(record);
        std::streamoff sz = record.size;
        record.blob.resize(sz);
        stream.read((char *)record.blob.data(), record.blob.size());
    }

    // Decode a dynamic type using the description name.  This name will become
    // decode() as soon as I figure out how to distingish strings from !hana::Foldable<>.
    node decode_desc(const std::string& type, bool endian = false);

    node decode(const descriptor::type&);

    // Define a set of factory functions that know how to decode specific binary
    // types.  They keys are strings from the "type" field of the TOML descriptions.
    using parser = std::function<node (decoder&)>;
    static std::map<std::string, parser> parse_map;

    // Factory function of any supported type, for convenience
    template <typename T>
    T decode() {
        T value;
        decode(value);
        return std::move(value);
    }

    // Deserialize a structure from a known offset in the file
    template <typename T>
    T decode(std::streamoff pos) {
        stream.seekg(pos);
        return decode<T>();
    }

    logging::logger log { "decoder" };

// protected:

    std::istream& stream;
    std::streamoff endpos;
};

inline log_record iterator::operator*() { 
    log_record record = stream->decode<log_record>(pos); 
    // record.offset.begin = pos;
    // record.offset.end = pos + record.size + descriptor::size<log_record>::value();
    return record;
}

inline iterator& iterator::operator++() {
    
    // Advance the iterator's position to the beginning of the next record.
    // Keep going as long as the filter returns false.
    do {
        pos += descriptor::size<log_record>::value() + stream->decode<log_record>(pos).size;
    } while (pos < end && !filter(*this));

    return *this;
}

inline log_record iterator::operator->() { 
    return operator*(); 
}

}} // namespace polysync::plog
