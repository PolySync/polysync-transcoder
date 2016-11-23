#include <set>

#include <polysync/plog/encoder.hpp>

namespace polysync { namespace plog {

// Match all integer types to swap the bytes; specialize for non-integers below.
template <typename T>
typename std::enable_if<std::is_integral<T>::value, T>::type
byteswap(const T& value) {
    endian::endian_arithmetic<endian::order::big, T, 8*sizeof(T)> swap(value);
    return *(new ((void *)swap.data()) T);
}

float byteswap(float value) {
    // Placement new native float bytes into an int
    std::uint32_t bytes = *(new ((void *)&value) std::uint32_t);
    endian::big_uint32_t swap(bytes);
    // Placement new back into a byte-swapped float
    return *(new ((void *)swap.data()) float);
}

double byteswap(double value) {
    // Placement new native float bytes into an int
    std::uint64_t bytes = *(new ((void *)&value) std::uint64_t);
    endian::big_uint64_t swap(bytes);
    // Placement new back into a byte-swapped double
    return *(new ((void *)swap.data()) double);
}

// Non-arithmetic types should not ever be marked bigendian.
template <typename T>
typename std::enable_if<!std::is_arithmetic<T>::value, T>::type
byteswap(const T&) {
    throw polysync::error("non-arithmetic type cannot be byteswapped");
}

struct branch {
    encoder* enc;
    const polysync::node& node;
    const polysync::tree tree;
    const descriptor::field& field;

    // Terminal types
    void operator()(std::type_index idx) const {
        if (!descriptor::typemap.count(idx))
            throw polysync::error("no typemap") 
                << exception::field(field.name)
                << exception::module("plog::encode");

        if (idx != node.target_type())
            throw polysync::error("mismatched type")
               << exception::field(node.name)
               << exception::module("plog::encode");

        BOOST_LOG_SEV(enc->log, severity::debug2) << node.name << " = " << node 
            << " (" << descriptor::typemap.at(idx).name << ")";
        eggs::variants::apply([this](auto val) { 
                if (field.bigendian)
                    enc->encode(byteswap(val));
                else
                    enc->encode(val);
                }, node);
    }

    void operator()(const descriptor::nested& idx) const {
        const polysync::tree* nest = node.target<polysync::tree>();
        if (nest == nullptr)
            throw polysync::error("mismatched nested type")
               << exception::field(node.name)
               << exception::module("plog::encode")
               ;

        std::string type = field.type.target<descriptor::nested>()->name;
        if (!descriptor::catalog.count(type))
            throw polysync::error("unknown type")
                << exception::type(type)
                << exception::field(node.name);

        BOOST_LOG_SEV(enc->log, severity::debug2) << node.name << " = " << node << " (nested)";
        enc->encode(*nest, descriptor::catalog.at(type));
    }

    void operator()(const descriptor::skip& skip) const {
        const bytes& raw = *node.target<bytes>();
        enc->stream.write((char *)raw.data(), raw.size());

        // eggs::variants::apply([this](auto val) { enc->encode(val); }, node);

        // std::string pad(skip.size, 0);
        // enc->stream.write(pad.c_str(), skip.size);
        BOOST_LOG_SEV(enc->log, severity::debug2) << "padded " << raw;
    }

    template <typename T>
    static void array(encoder* enc, const polysync::node& node, size_t size) {
        const std::vector<T>* arr = node.target<std::vector<T>>();
        if (arr == nullptr)
            throw polysync::error("mismatched type in field \"" + node.name + "\"");

        if (arr->size() != size)
            throw polysync::error("mismatched size in field \"" + node.name + "\"");

        std::for_each(arr->begin(), arr->end(), [enc](const auto& val) { enc->encode(val); });
    }

    static std::map<std::type_index, std::function<void (encoder*, const polysync::node&, size_t)>> array_func;
    void operator()(const descriptor::array& desc) const {
        auto sizefield = desc.size.target<std::string>();
        auto fixedsize = desc.size.target<size_t>();
        size_t size;

        if (sizefield) {
            auto it = std::find_if(tree->begin(), tree->end(), 
                    [sizefield](const polysync::node& n) { return n.name == *sizefield; }); 
            if (it == tree->end())
                throw polysync::error("size indicator field not found") << exception::field(*sizefield);

            // Figure out the size, irregardless of the integer type
            std::stringstream os;
            os << *it;
            try {
                size = std::stoll(os.str());
            } catch (std::invalid_argument) {
                std::cout << it->name << std::endl;
                throw polysync::error("cannot parse size") << exception::type(it->name);
            }
        } else
            size = *fixedsize; 

        BOOST_LOG_SEV(enc->log, severity::debug2) << "encoding " << size << " elements";
        auto nesttype = desc.type.target<std::string>();
        
        if (nesttype) {
            if (!descriptor::catalog.count(*nesttype))
                throw polysync::error("unknown nested type");

            const descriptor::type& nest = descriptor::catalog.at(*nesttype);
            const std::vector<polysync::tree>* arr = node.target<std::vector<polysync::tree>>();

            // Actual data type is not a vector like the description requires.
            if (arr == nullptr)
                throw polysync::error("mismatched type") << exception::field(node.name);

            std::for_each(arr->begin(), arr->end(), [this, nest](auto elem) {
                    enc->encode(elem, nest);
                    });
        } else {
            std::type_index idx = *desc.type.target<std::type_index>();
            if (!array_func.count(idx))
                throw polysync::error("unknown terminal type");
            return array_func.at(idx)(enc, node, size);
        }
    }

};

std::map<std::type_index, std::function<void (encoder*, const polysync::node&, size_t)>> 
    branch::array_func {
        { typeid(float), branch::array<float> }, 
        { typeid(double), branch::array<double> }, 
        { typeid(std::int8_t), branch::array<std::int8_t> }, 
        { typeid(std::int16_t), branch::array<std::int16_t> }, 
        { typeid(std::int32_t), branch::array<std::int32_t> }, 
        { typeid(std::int64_t), branch::array<std::int64_t> }, 
        { typeid(std::uint8_t), branch::array<std::uint8_t> }, 
        { typeid(std::uint16_t), branch::array<std::uint16_t> }, 
        { typeid(std::uint32_t), branch::array<std::uint32_t> }, 
        { typeid(std::uint64_t), branch::array<std::uint64_t> }, 
    };

void encoder::encode( const tree& t, const descriptor::type& desc ) {

    std::set<std::string> done;

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

            eggs::variants::apply(branch { this, *fi, t, field }, field.type);
            done.insert(field.name);
            });

    // What to do with fields not described in desc?  For now, the policy is to
    // omit terminals, but encode any trees, in order.  This should work when
    // the type description changes by removing a terminal field, but still
    // completes the encoding of a list of trees. 
    std::for_each(t->begin(), t->end(), [&](const node& n) {
            if (done.count(n.name)) 
                return;
            if (n.target_type() == typeid(tree)) {
                tree subtree = *n.target<tree>();
                const descriptor::type& subtype = descriptor::catalog.at(subtree.type);
                BOOST_LOG_SEV(log, severity::debug1) << "recursing subtree " << subtree.type;
                return encode(subtree, subtype);
            } 
            BOOST_LOG_SEV(log, severity::debug2) 
                << "field \"" << n.name << "\" not serialized due to lack of description";

            });
}


}} // namespace polysync::plog
