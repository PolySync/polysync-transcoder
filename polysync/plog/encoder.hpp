# pragma once

#include <polysync/plog/core.hpp>
#include <polysync/plog/io.hpp>
#include <polysync/exception.hpp>
#include <fstream>
#include <boost/hana.hpp>

namespace polysync { namespace plog {

namespace hana = boost::hana;
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

    struct branch_encoder {
        encoder* enc;
        const plog::node& node;
        const descriptor::field& field;

        void operator()(std::type_index idx) const {
            if (!descriptor::typemap.count(idx))
                throw polysync::error("unknown type for field \"" + node.name + "\"");

            if (idx != node.target_type())
                throw polysync::error("mismatched type in field \"" + node.name + "\"");

            eggs::variants::apply([this](auto val) { enc->encode(val); }, node);
        }

        void operator()(const descriptor::nested& idx) const {
            const plog::tree* nest = node.target<plog::tree>();
            if (nest == nullptr)
                throw polysync::error("mismatched type in field \"" + node.name + "\"");

            std::string type = field.type.target<descriptor::nested>()->name;
            if (!descriptor::catalog.count(type))
                throw polysync::error("unknown type \"" + type + "\" for field \"" + node.name + "\"");

            enc->encode(*nest, descriptor::catalog.at(type));
        }

        void operator()(const descriptor::skip& skip) const {
            std::string pad(skip.size, 0);
            enc->stream.write(pad.c_str(), skip.size);
            BOOST_LOG_SEV(enc->log, severity::debug2) << "padded " << skip.size << " bytes";
        }

        template <typename T>
        static void array(encoder* enc, const plog::node& node, size_t size) {
            const std::vector<T>* arr = node.target<std::vector<T>>();
            if (arr == nullptr)
                throw polysync::error("mismatched type in field \"" + node.name + "\"");

            if (arr->size() != size)
                throw polysync::error("mismatched size in field \"" + node.name + "\"");

            std::for_each(arr->begin(), arr->end(), [enc](const auto& val) { enc->encode(val); });
        }

        void operator()(const descriptor::terminal_array& desc) const {
            std::map<std::type_index, std::function<void (encoder*, const plog::node&, size_t)>> 
                array_func {
                    { typeid(std::uint8_t), branch_encoder::array<std::uint8_t> }, 
                    { typeid(std::uint16_t), branch_encoder::array<std::uint16_t> }, 
                };

                if (!array_func.count(desc.type))
                    throw polysync::error("unknown terminal type");

                return array_func.at(desc.type)(enc, node, desc.size);
        }

        void operator()(const descriptor::nested_array& idx) const {
            const std::vector<plog::tree>* arr = node.target<std::vector<tree>>();

            // Actual data type is not a vector like the description requires.
            if (arr == nullptr)
                throw polysync::error("mismatched type in field \"" + node.name + "\"");

            if (idx.desc.empty())
                throw polysync::error("empty type descriptor \""  "\" for field \"" + field.name + "\"");

            std::for_each(arr->begin(), arr->end(), [this, idx](auto elem) {
                    enc->encode(elem, idx.desc);
                    });
        }
    };

public:

    void encode(const plog::node& n) {
        BOOST_LOG_SEV(log, severity::debug1) << "encoding " << n.name;
        eggs::variants::apply([this](auto value) { encode(value); }, n);
    }

public:
    
    logging::logger log { "plog::encoder" };

protected:

    std::ostream& stream;
    std::map<plog::msg_type, std::string> msg_type_map;

};

inline void encoder::encode(const tree& t, const descriptor::type& desc) {

        // The serialization must be in the order of the descriptor, not
        // necessarily the value, so iterate the descriptor at the top level.
        std::for_each(desc.begin(), desc.end(), [&](const descriptor::field& field) {

                // Search the tree itself to find the field, keyed on their
                // names; The tree's natural order is irrelevant, given a
                // descriptor.  This is exactly how the type can be re-ordered
                // during a format change without breaking legacy plogs.
                auto fi = std::find_if(t->begin(), t->end(), [field](const node& n) { 
                        return n.name == field.name; 
                        });

                // Skip is a special case and will never be in the parse tree,
                // although it is still a critical part of the description.
                if (fi == t->end() && field.name != "skip")
                    throw polysync::error("field \"" + field.name + "\" not found in tree");

                eggs::variants::apply(branch_encoder { this, *fi, field }, field.type);

        });
    }

 
}} // namespace polysync::plog
