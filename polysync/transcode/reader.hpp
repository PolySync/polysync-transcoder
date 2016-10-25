#pragma once

#include <polysync/transcode/core.hpp>
#include <polysync/transcode/size.hpp>
#include <polysync/transcode/description.hpp>
#include <polysync/transcode/logging.hpp>
#include <fstream>
#include <boost/hana.hpp>

namespace polysync { namespace plog {

namespace hana = boost::hana;

// Indirecting and incrementing an iterator requires reading the plog file
// itself, which we cannot do until plog::reader is defined.  So implement
// these functions below, after reader definition.  In the meantime, forward
// declare reader so we can prototype the methods.
struct reader;

// Define an STL compatible iterator to traverse plog files
struct iterator {

    reader* stream;
    std::streamoff pos; // file offset from std::ios_base::beg, pointing to next record
    std::streamoff end; // filters need to know where the end is
    std::function<bool (iterator)> filter;

    bool operator!=(const iterator& other) const { return pos != other.pos; }
    bool operator==(const iterator& other) const { return pos == other.pos; }

    log_record operator*(); // read and return the payload
    iterator& operator++(); // skip to the next record

    log_record operator->();
};

class reader {
public:

    reader(std::istream& st) : stream(st) {
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
    // Define a set of read() templates, overloads, and specializations to pattern
    // match every possible struct, sequence, or terminal type, nested or not,
    // that is possibly found in a message.

    // For flat objects like arithmetic types, just straight copy memory as
    // blob to file. In particular, most types that are not hana structures are these.
    template <typename Number>
    typename std::enable_if_t<!hana::Foldable<Number>::value>
    read(Number& value) {
        stream.read((char *)(&value), sizeof(Number)); 
    }
    
    // Specialize read() for hana wrapped structures.  This works out padding
    // in the structure definition where a straight memcpy would fail, and also
    // recurses into nested structures.  Hana has a function hana::members()
    // which would make this simpler, but sadly members() cannot return
    // non-const references which we need here (we are setting the value).
    template <typename Record, class = typename std::enable_if_t<hana::Foldable<Record>::value>>
    void read(Record& record) {
        hana::for_each(hana::keys(record), [&](auto&& key) mutable { 
                read(hana::at_key(record, key));
                }); 
    }

    // Sequences have a LenType number first specifying how many T's follow.  T
    // might be a flat (arithmetic) type or a nested structure.  Either way,
    // iterate the fields and serialize each one using the specialized read().
    template <typename LenType, typename T> 
    void read(sequence<LenType, T>& seq) {
        LenType len;
        stream.read(reinterpret_cast<char *>(&len), sizeof(len));
        seq.resize(len);
        std::for_each(seq.begin(), seq.end(), [=](auto&& val) mutable { read(val); });
    }

    // Specialize name_type because the underlying std::string type needs special
    // handling.  It resembles a Pascal string (length first, no trailing zero
    // as a C string would have)
    void read(name_type& name) {
        std::uint16_t len;
        stream.read((char *)(&len), sizeof(len));
        name.resize(len);
        stream.read((char *)(name.data()), len); 
    }

    // Specialize log_record because the binary blob needs special handling
    void read(log_record& record) {
        read<log_record>(record);
        std::streamoff sz = record.size;
        record.blob.resize(sz);
        stream.read((char *)record.blob.data(), record.blob.size());
    }

    // Factory function of any supported type, for convenience
    template <typename T>
    T read() {
        T value;
        read(value);
        return std::move(value);
    }

    // Deserialize a structure from a known offset in the file
    template <typename T>
    T read(std::streamoff pos) {
        stream.seekg(pos);
        return read<T>();
    }

    logging::logger log { "reader" };

// protected:

    std::istream& stream;
    std::streamoff endpos;
};

inline log_record iterator::operator*() { 
    return stream->read<log_record>(pos); 
}

inline iterator& iterator::operator++() {
    
    // Advance the iterator's position to the beginning of the next record.
    // Keep going as long as the filter returns false.
    do {
        pos += size<log_record>::value() + stream->read<log_record>(pos).size;
    } while (pos < end && !filter(*this));

    return *this;
}

inline log_record iterator::operator->() { 
    return stream->read<log_record>(pos); 
}

}} // namespace polysync::plog
