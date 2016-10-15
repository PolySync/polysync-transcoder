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
    // Define a set of write() overloads to pattern match every possible
    // struct, sequence, or terminal type, nested or not, that is possibly
    // found in a message.

    // Write non-flat (hana wrapped) structures.  This works out packing
    // the structure where a straight memcpy would fail due to padding, and also
    // recurses into nested structures.
    template <typename Struct>
    typename std::enable_if_t<hana::Foldable<Struct>::value>
    write(const Struct& rec) {
        hana::for_each(hana::members(rec), [this](auto field) { write(field); });
    }

    // For flat objects like arithmetic types, just straight copy memory as
    // blob to file. In particular, most types that are not hana structures are these.
    template <typename Number>
    typename std::enable_if_t<!hana::Foldable<Number>::value>
    write(const Number& value) {
        plog.write((char *)(&value), sizeof(Number)); 
    }
    
    // Write raw buffer, as bytes of length sz.
    template <typename T>
    void write(const T& buf, size_t sz) {
        plog.write((char *)&buf, sz);
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

    // Fixed length arrays modeled by std::array<>.
    template <typename... Args>
    void write(const std::array<Args...>& array) {
        plog.write((char *)array.data(), array.size()*sizeof(decltype(array)::value_type)); 
    }

    // plog::hash_type is multiprecision; other very long ints may come along someday
    template <typename... Args>
    void write(const multiprecision::number<Args...>& value) {
        std::ostream_iterator<std::uint8_t> it(plog);
        multiprecision::export_bits(value, it, 8);
    }
   
    // Specialize name_type because the underlying std::string type needs special
    // handling.  It resembels a Pascal string (length first, no trailing zero
    // as a C string would have)
    void write(const name_type& name) {
        name_type::length_type len = name.size();
        plog.write((char *)(&len), sizeof(len));
        plog.write((char *)(name.data()), len); 
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

protected:

    std::ostream& plog;
    std::map<plog::msg_type, std::string> msg_type_map;

};
 
}} // namespace polysync::plog
