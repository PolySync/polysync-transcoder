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
// itself, which we cannot do until plog::decoder is defined.  So implement
// these functions below, after decoder definition.  In the meantime, forward
// declare decoder so we can prototype the methods.
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
    iterator& operator++(); // skip to the next record

    log_record operator->();
};

// Dynamic parsing builds a tree of nodes to represent the record.  Each leaf
// is strongly typed as one of the variant component types.
struct node : variant {

    template <typename T>
    node(const T& value, std::string n) : variant(value), name(n) { }

    using variant::variant;
    std::string name;

    template <typename Struct>
    static node from(const Struct&, const std::string& name);
 };

struct tree : std::vector<node> {};

// Convert a hana structure into a vector of dynamic nodes.
template <typename Struct>
inline node node::from(const Struct& s, const std::string& name) {
    std::shared_ptr<tree> tr = std::make_shared<tree>();
    hana::for_each(s, [tr](auto pair) { 
            tr->emplace_back(hana::second(pair), hana::to<char const*>(hana::first(pair)));
            });
        
    return node(tr, name);
}

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
    template <typename Record, class = typename std::enable_if_t<hana::Foldable<Record>::value>>
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

    std::string detect(const node& parent); // Detect the next type from parent decode
    node decode(const plog::log_record&);
    node decode_desc(const std::string& type);

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
    return stream->decode<log_record>(pos); 
}

inline iterator& iterator::operator++() {
    
    // Advance the iterator's position to the beginning of the next record.
    // Keep going as long as the filter returns false.
    do {
        pos += size<log_record>::value() + stream->decode<log_record>(pos).size;
    } while (pos < end && !filter(*this));

    return *this;
}

inline log_record iterator::operator->() { 
    return stream->decode<log_record>(pos); 
}

}} // namespace polysync::plog
