# pragma once

#include <polysync/transcode/core.hpp>
#include <polysync/transcode/io.hpp>

#include <boost/filesystem.hpp>
#include <boost/hana.hpp>

// Define an STL compatible iterator to traverse plog files

namespace polysync { namespace plog {

namespace fs = boost::filesystem;
namespace hana = boost::hana;

// Indirecting and incrementing an iterator requires reading the plog file
// itself, which we cannot do until plog::reader is defined.  So implement
// these functions below, after reader definition.  In the meantime, forward
// declare reader so we can prototype the methods.

struct reader;

struct iterator {

    reader* plog;
    std::streamoff pos; // file offset from std::ios_base::beg, pointing to next record
    std::function<bool (iterator)> filter;

    bool operator!=(const iterator& other) const { return pos != other.pos; }
    bool operator==(const iterator& other) const { return pos == other.pos; }

    record_type operator*(); // read and return the payload
    iterator& operator++(); // skip to the next record

    record_type operator->();
};

class reader {
public:
    // Factory methods for STL compatible iterators

    iterator begin(std::function<bool (iterator)> filt) { return iterator { this, plog.tellg(), filt }; }
    iterator end() { return iterator { this, endpos }; }

    // Constructors (why can't this be inline?)
    reader(const std::string& path);

    // Getters
    const log_header& get_header() const { return header; }
    const std::string& get_filename() const { return filename; }

private:

    friend struct iterator;

    // For flat objects like arithmetic types, just straight copy file blob into the memory
    template <typename Number>
    typename std::enable_if_t<!hana::Foldable<Number>::value>
    read(Number& record) {
        plog.read(reinterpret_cast<char *>(&record), sizeof(Number)); 
    }
    
    // Specialize read() for hana wrapped structures.  This works out padding
    // in the structure definition where a straight memcpy would fail, and also
    // recurses into nested structures.  Hana has a function hana::members()
    // which would make this simpler, but sadly members() cannot return
    // non-const references which we need here (we are setting the value).
    template <typename Record>
    void blind_read(Record& record) {
        hana::for_each(hana::keys(record), [&](auto&& key) mutable { 
                read(hana::at_key(record, key));
                }); 
    }

    template <typename Record>
    void checked_read(Record& record) {
    }

    template <typename Record, class = typename std::enable_if_t<hana::Foldable<Record>::value>>
    void read(Record& record) {
        // if (hana::contains(m, Record))
        blind_read(record);
    }

    // Sequences have a LenType number first specifying how many T's follow.
    template <typename LenType, typename T> 
    void read(sequence<LenType, T>& seq) {
        LenType len;
        plog.read(reinterpret_cast<char *>(&len), sizeof(LenType));
        seq.resize(len);
        std::for_each(seq.begin(), seq.end(), [=](auto&& val) mutable { read(val); });
    }

    // name_type is a specialized sequence that is like a Pascal string (length
    // first, no trailing zero as a C string would have)
    void read(name_type& name) {
        std::uint16_t len;
        plog.read(reinterpret_cast<char *>(&len), sizeof(len));
        name.resize(len);
        plog.read((char *)(name.data()), len); 
    }


    // Deserialize a structure from a known offset in the file
    template <typename Struct>
    Struct read(std::streamoff pos) {
        plog.seekg(pos);
        Struct record;
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

inline record_type iterator::operator*() { 
    return plog->read<log_record>(pos); 
}

inline iterator& iterator::operator++() {
    
    // Advance the iterator's position to the beginning of the next record.
    // This number is the packed storage size of log_record + payload size read
    // from log_record.size.
    pos += packed_size<log_record>() + plog->read<log_record>(pos).size; 

    return *this;
}

inline record_type iterator::operator->() { 
    return plog->read<log_record>(pos); 
}

}} // namespace polysync::plog
