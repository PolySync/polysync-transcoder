#include <polysync/plog/encoder.hpp>

namespace polysync { namespace plog {

struct branch {
    encoder* enc;
    const plog::node& node;
    const plog::tree tree;
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

    static std::map<std::type_index, std::function<void (encoder*, const plog::node&, size_t)>> array_func;
    void operator()(const descriptor::array& desc) const {
        auto sizefield = desc.size.target<std::string>();
        auto fixedsize = desc.size.target<size_t>();
        size_t size;

        if (sizefield) {
            auto it = std::find_if(tree->begin(), tree->end(), 
                    [sizefield](const plog::node& n) { return n.name == *sizefield; }); 
            if (it == tree->end())
                throw polysync::error("size indicator field not found") << exception::field(*sizefield);

            // Figure out the size, irregardless of the integer type
            std::stringstream os;
            os << *it;
            size = std::stoll(os.str());
        } else
            size = *fixedsize; 

        BOOST_LOG_SEV(enc->log, severity::debug2) << "encoding " << size << " elements";
        auto nesttype = desc.type.target<std::string>();
        
        if (nesttype) {
            if (!descriptor::catalog.count(*nesttype))
                throw polysync::error("unknown nested type");

            const descriptor::type& nest = descriptor::catalog.at(*nesttype);
            const std::vector<plog::tree>* arr = node.target<std::vector<plog::tree>>();

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

std::map<std::type_index, std::function<void (encoder*, const plog::node&, size_t)>> 
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


void encoder::encode(const tree& t, const descriptor::type& desc) {

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

        });
    }


}} // namespace polysync::plog
