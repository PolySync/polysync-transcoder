# pragma once

#include <polysync/transcode/core.hpp>
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

    reader* plog;
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
    // Factory methods for STL compatible iterators

    iterator begin(std::function<bool (iterator)> filt) { 
        return iterator { this, plog.tellg(), endpos, filt }; 
    }

    iterator end() { return iterator { this, endpos, endpos }; }

    // Constructors (why can't this be inline?)
    reader(const std::string& path);

    // Getters
    const log_header& get_header() const { return header; }
    const std::string& get_filename() const { return filename; }

public:
    // Define a set of read() templates, overloads, and specializations to pattern
    // match every possible struct, sequence, or terminal type, nested or not,
    // that is possibly found in a message.

    // For flat objects like arithmetic types, just straight copy memory as
    // blob to file. In particular, most types that are not hana structures are these.
    template <typename Number>
    typename std::enable_if_t<!hana::Foldable<Number>::value>
    read(Number& value) {
        plog.read((char *)(&value), sizeof(Number)); 
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
        plog.read(reinterpret_cast<char *>(&len), sizeof(len));
        seq.resize(len);
        std::for_each(seq.begin(), seq.end(), [=](auto&& val) mutable { read(val); });
    }

    // Specialize name_type because the underlying std::string type needs special
    // handling.  It resembels a Pascal string (length first, no trailing zero
    // as a C string would have)
    void read(name_type& name) {
        std::uint16_t len;
        plog.read((char *)(&len), sizeof(len));
        name.resize(len);
        plog.read((char *)(name.data()), len); 
    }

    // Specialize log_record because the binary blob needs special handling
    void read(log_record& record) {
        read<log_record>(record);
        std::streamoff sz = record.size - size<msg_header>::packed();
        record.blob.resize(sz);
        plog.read(reinterpret_cast<char *>(record.blob.data()), record.blob.size());
    }


    // Deserialize a structure from a known offset in the file
    log_record read(std::streamoff pos) {
        plog.seekg(pos);
        log_record record;
        read(record);
        return std::move(record);
    }

protected:

    const std::string filename;
    mutable std::ifstream plog;
    std::streamoff endpos;
    log_header header;
};

inline reader::reader(const std::string& path) : filename(path), plog(path, std::ifstream::binary) {
    plog.seekg(0, std::ios_base::end);
    endpos = plog.tellg();
    plog.seekg(0, std::ios_base::beg);
    read(header);
}

inline log_record iterator::operator*() { 
    return plog->read(pos); 
}

inline iterator& iterator::operator++() {
    
    // Advance the iterator's position to the beginning of the next record.
    // Keep going as long as the filter returns false.
    do {
        pos += size<log_record>::packed() - size<msg_header>::packed() + plog->read(pos).size;
    } while (pos < end && !filter(*this));

    return *this;
}

inline log_record iterator::operator->() { 
    return plog->read(pos); 
}

}} // namespace polysync::plog
