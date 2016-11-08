#include <polysync/plog/encoder.hpp>

namespace polysync { namespace plog {

struct branch {
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

    static std::map<std::type_index, std::function<void (encoder*, const plog::node&, size_t)>> array_func;
    void operator()(const descriptor::terminal_array& desc) const {
            if (!array_func.count(desc.type))
                throw polysync::error("unknown terminal type");

            return array_func.at(desc.type)(enc, node, desc.size);
    }

    void operator()(const descriptor::dynamic_array& ta) const {
        // The branch should have a previous element with name ta.sizename
        const plog::tree& branch = *node.target<plog::tree>();
        auto it = std::find_if(branch->begin(), branch->end(), [ta](const plog::node& n) {
                return n.name == ta.sizefield; }); 
        if (it == branch->end())
            throw polysync::error("size indicator field \"" + ta.sizefield + "\" was not found");

        std::uint16_t* size = it->target<std::uint16_t>();
        if (!size)
            throw polysync::error("cannot determine array size");

        return array_func.at(ta.type)(enc, node, (size_t)size);
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

                eggs::variants::apply(branch { this, *fi, field }, field.type);

        });
    }


}} // namespace polysync::plog
