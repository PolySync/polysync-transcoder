# pragma once

#include <polysync/transcode/core.hpp>
#include <fstream>
#include <boost/hana.hpp>

namespace polysync { namespace plog {

namespace hana = boost::hana;

class writer {
public:

    writer(std::ostream& st) : plog(st) {} 

public:
    // Define a set of write() templates, overloads, and specializations to pattern
    // match every possible struct, sequence, or terminal type, nested or not,
    // that is possibly found in a message.

    // For flat objects like arithmetic types, just straight copy memory as
    // blob to file. In particular, most types that are not hana structures are these.
    template <typename Number>
    typename std::enable_if_t<!hana::Foldable<Number>::value>
    write(const Number& value) {
        std::cout << "Number " << sizeof(Number) << " " << value << std::endl;
        plog.write((char *)(&value), sizeof(Number)); 
    }
    
    void write(const std::array<std::uint8_t, 8>& array) {
        std::cout << "array<8>" << std::endl;
        plog.write((char *)array.data(), 8*sizeof(std::uint8_t)); 
    }

    void write(const hash_type& value) {
        std::cout << "hash_type " << value << std::endl;
        std::ostream_iterator<std::uint8_t> it(plog);
        multiprecision::export_bits(value, it, 8);
    }

    // Write non-flat (hana wrapped) structures.  This works out packing
    // the structure where a straight memcpy would fail due to padding, and also
    // recurses into nested structures.
    template <typename Struct>
    typename std::enable_if_t<hana::Foldable<Struct>::value>
    write(const Struct& rec) {
        hana::for_each(hana::members(rec), [this](auto field) { write(field); });
    }
    
    // Sequences have a LenType number first specifying how many T's follow.  T
    // might be a flat (arithmetic) type or a nested structure.  Either way,
    // iterate the fields and serialize each one using the specialized write().
    template <typename LenType, typename T> 
    void write(const sequence<LenType, T>& seq) {
        LenType len = seq.size();
        plog.write((char *)(&len), sizeof(len));
        std::for_each(seq.begin(), seq.end(), [this](auto val) { write(val); });
    }

    // Specialize name_type because the underlying std::string type needs special
    // handling.  It resembels a Pascal string (length first, no trailing zero
    // as a C string would have)
    void write(const name_type& name) {
        name_type::length_type len = name.size();
        plog.write((char *)(&len), sizeof(len));
        plog.write((char *)(name.data()), len); 
    }

    // Specialize log_record because the binary blob needs special handling
    void write(const log_record& record) {
        write<log_record>(record);
        // plog.write((char *)(record.blob.data()), record.blob.size());
    }

    void write(const field_descriptor& field, std::vector<std::uint8_t>::const_iterator& it) {
        char *ptr = (char *)&(*it);
        size_t sz = plog::size<field_descriptor>(field).packed();
        plog.write(ptr, sz);
        it += sz;
    }

public:
    // Generate self descriptions of types 
    
    template <typename Record>
    std::string describe() const {
        if (!static_typemap.count(typeid(Record)))
            throw std::runtime_error("no typemap description");
        std::string tpname = static_typemap.at(typeid(Record)).name; 
        std::string result = tpname + " { ";
        hana::for_each(Record(), [&result, tpname](auto pair) {
                std::type_index tp = typeid(hana::second(pair));
                std::string fieldname = hana::to<char const*>(hana::first(pair));
                if (static_typemap.count(tp) == 0)
                    throw std::runtime_error("type not described for field \"" + tpname + "::" + fieldname + "\"");
                result += fieldname + ": " + static_typemap.at(tp).name + " " + std::to_string(static_typemap.at(tp).size) +  "; ";
                });
        return result + "}";
    }

// private:

    std::ostream& plog;
    std::map<plog::ps_msg_type, std::string> msg_type_map;

};
 
}} // namespace polysync::plog
