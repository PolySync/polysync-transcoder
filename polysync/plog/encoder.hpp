# pragma once

#include <polysync/plog/core.hpp>
#include <polysync/io.hpp>
#include <polysync/exception.hpp>
#include <fstream>
#include <boost/hana.hpp>
#include <boost/endian/arithmetic.hpp>

namespace polysync { namespace plog {

namespace hana = boost::hana;
namespace endian = boost::endian;
using logging::severity;

class encoder {
public:

    encoder(std::ostream& st) : stream(st) {} 

    // Expose the encode() members as operator(), for convenience.
    template <typename... Args>
    void operator()(Args... args) { encode(std::forward<Args>(args)...); }

public:
    // Define a set of encode() overloads to pattern match every possible
    // struct, sequence, or terminal type, nested or not, that is possibly
    // found in a message.

    // Write non-flat (hana wrapped) structures.  This works out packing
    // the structure where a straight memcpy would fail due to padding, and also
    // recurses into nested structures.
    template <typename Struct>
    typename std::enable_if_t<hana::Foldable<Struct>::value>
    encode(const Struct& rec) {
        hana::for_each(hana::members(rec), [this](auto field) { encode(field); });
    }

    // For flat objects like arithmetic types, just straight copy memory as
    // blob to file. In particular, most types that are not hana structures are these.
    template <typename Number>
    typename std::enable_if_t<!hana::Foldable<Number>::value>
    encode(const Number& value) {
        stream.write((char *)(&value), sizeof(Number)); 
    }
    
    // Endian swapped types
    template <typename T, size_t N>
    void encode(const boost::endian::endian_arithmetic<boost::endian::order::big, T, N>& value) {
        stream.write((char *)value.data(), sizeof(T));
    }

    // Write raw buffer, as bytes of length sz.
    template <typename T>
    void encode(const T& buf, size_t sz) {
        stream.write((char *)&buf, sz);
    }

    // Sequences have a LenType number first specifying how many T's follow.  T
    // might be a flat (arithmetic) type or a nested structure.  Either way,
    // iterate the fields and serialize each one using the specialized encode().
    template <typename LenType, typename T> 
    void encode(const sequence<LenType, T>& seq) {
        LenType len = seq.size();
        stream.write((char *)(&len), sizeof(len));
        std::for_each(seq.begin(), seq.end(), [this](auto val) { encode(val); });
    }

    // Fixed length arrays modeled by std::array<>.
    template <typename... Args>
    void encode(const std::array<Args...>& array) {
        stream.write((char *)array.data(), array.size()*sizeof(decltype(array)::value_type)); 
    }

    // plog::hash_type is multiprecision; other very long ints may come along someday
    template <typename... Args>
    void encode(const multiprecision::number<Args...>& value) {
        std::ostream_iterator<std::uint8_t> it(stream);
        multiprecision::export_bits(value, it, 8);
    }
   
    // Specialize name_type because the underlying std::string type needs special
    // handling.  It resembels a Pascal string (length first, no trailing zero
    // as a C string would have)
    void encode(const name_type& name) {
        name_type::length_type len = name.size();
        stream.write((char *)(&len), sizeof(len));
        stream.write((char *)(name.data()), len); 
    }

    void encode(const plog::tree& t, const descriptor::type& desc);

public:

    void encode(const plog::node& n) {
        BOOST_LOG_SEV(log, severity::debug1) << "encoding " << n.name;
        eggs::variants::apply([this](auto value) { encode(value); }, n);
    }

public:
    
    logging::logger log { "plog::encoder" };

protected:
    friend class branch;

    std::ostream& stream;
    std::map<plog::msg_type, std::string> msg_type_map;

};

 
}} // namespace polysync::plog
