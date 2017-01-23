#include <set>

#include <polysync/print_tree.hpp>
#include <polysync/byteswap.hpp>
#include <polysync/encoder.hpp>

namespace polysync {

struct branch
{
    Encoder* enc;
    const polysync::Node& node;
    const polysync::Tree tree;
    const descriptor::Field& field;

    // Terminal types
    void operator()( std::type_index idx ) const
    {
        if ( !descriptor::terminalTypeMap.count(idx) )
        {
            throw polysync::error( "no typemap" )
                << exception::field( field.name )
                << exception::module( "encoder" );
        }
        if ( idx != node.target_type() )
        {
            throw polysync::error( "mismatched type" )
               << exception::field( node.name )
               << exception::module( "encoder" );
        }

        BOOST_LOG_SEV( enc->log, severity::debug2 )
            << node.name << " = " << node
            << " (" << descriptor::terminalTypeMap.at(idx).name << ")";

        eggs::variants::apply( [this]( auto& val ) {
                switch ( field.byteorder )
                {
                    case descriptor::ByteOrder::BigEndian:
                        enc->encode( byteswap(val) );
                        break;
                    case descriptor::ByteOrder::LittleEndian:
                        enc->encode(val);
                        break;
                } }, node);
    }

    void operator()(const descriptor::BitField& idx) const
    {
        throw polysync::error( "BitField not yet implemented" );
    }

    void operator()(const descriptor::Nested& idx) const
    {
        const polysync::Tree* nest = node.target<polysync::Tree>();
        if (nest == nullptr)
        {
            throw polysync::error("mismatched nested type")
               << exception::field(node.name)
               << exception::module("plog::encode");
        }

        std::string type = field.type.target<descriptor::Nested>()->name;
        if ( !descriptor::catalog.count(type) )
        {
            throw polysync::error( "unknown type" )
                << exception::type(type)
                << exception::field(node.name);
        }

        BOOST_LOG_SEV( enc->log, severity::debug2 ) << node.name << " = " << node << " (nested)";
        enc->encode( *nest, descriptor::catalog.at(type) );
    }

    void operator()( const descriptor::Skip& skip ) const
    {
        const Bytes& raw = *node.target<Bytes>();
        enc->stream.write( (char *)raw.data(), raw.size() );
        BOOST_LOG_SEV(enc->log, severity::debug2) << "padded " << raw;
    }

    template <typename T>
    static void array( Encoder* enc, const polysync::Node& node, size_t size )
    {
        const std::vector<T>* arr = node.target< std::vector<T> >();
        if (arr == nullptr)
        {
            throw polysync::error("mismatched type in field \"" + node.name + "\"");
        }
        if (arr->size() != size)
        {
            throw polysync::error("mismatched size in field \"" + node.name + "\"");
        }
        std::for_each( arr->begin(), arr->end(), [enc]( const auto& val ) { enc->encode(val); });
    }

    static std::map<std::type_index, std::function<void ( Encoder*, const polysync::Node&, size_t )> > array_func;

    void operator()( const descriptor::Array& desc ) const
    {
        auto sizefield = desc.size.target<std::string>();
        auto fixedsize = desc.size.target<size_t>();
        size_t size;

        if (sizefield)
        {
            auto it = std::find_if(tree->begin(), tree->end(),
                    [sizefield](const polysync::Node& n) { return n.name == *sizefield; });
            if ( it == tree->end() )
            {
                throw polysync::error( "size indicator field not found" ) << exception::field(*sizefield);
            }

            // Figure out the size, irregardless of the integer type
            std::stringstream os;
            os << *it;
            try
            {
                size = std::stoll(os.str());
            }
            catch ( std::invalid_argument )
            {
                throw polysync::error( "cannot parse size" ) << exception::type( it->name );
            }
        }
        else
        {
            size = *fixedsize;
        }

        BOOST_LOG_SEV( enc->log, severity::debug2 ) << "encoding " << size << " elements";
        auto nesttype = desc.type.target<std::string>();

        if ( nesttype )
        {
            if ( !descriptor::catalog.count(*nesttype) )
            {
                throw polysync::error("unknown nested type");
            }

            const descriptor::Type& nest = descriptor::catalog.at( *nesttype );
            const std::vector<polysync::Tree>* arr = node.target< std::vector<polysync::Tree> >();

            // Actual data type is not a vector like the description requires.
            if ( arr == nullptr )
            {
                throw polysync::error( "mismatched type" ) << exception::field( node.name );
            }
            std::for_each( arr->begin(), arr->end(), [this, nest]( auto& elem ) {
                    enc->encode( elem, nest );
                    });
        }
        else
        {
            std::type_index idx = *desc.type.target<std::type_index>();
            if (!array_func.count(idx))
            {
                throw polysync::error( "unknown terminal type" );
            }
            return array_func.at( idx )( enc, node, size );
        }
    }

};

std::map< std::type_index, std::function<void (Encoder*, const polysync::Node&, size_t)> >
    branch::array_func
    {
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

void Encoder::encode( const Tree& t, const descriptor::Type& desc )
{
    std::set<std::string> done;

    // The serialization must be in the order of the descriptor, not
    // necessarily the value, so iterate the descriptor at the top level.
    std::for_each( desc.begin(), desc.end(), [&](const descriptor::Field& field ) {

            // Search the tree itself to find the field, keyed on their
            // names; The tree's natural order is irrelevant, given a
            // descriptor.  This is exactly how the type can be re-ordered
            // during a format change without breaking legacy plogs.
            auto fi = std::find_if(t->begin(), t->end(), [field]( const Node& n ) {
                    return n.name == field.name;
                    });

            // Skip is a special case and will never be in the parse tree,
            // although it is still a critical part of the description.
            if ( fi == t->end() && field.name != "skip" )
            {
                throw polysync::error( "field \"" + field.name + "\" not found in tree" );
            }
            eggs::variants::apply( branch { this, *fi, t, field }, field.type );
            done.insert( field.name );
            });

    // What to do with fields not described in desc?  For now, the policy is to
    // omit terminals, but encode any trees or undecoded buffers, in order.  This should work when
    // the type description changes by removing a terminal field, but still
    // completes the encoding of a list of trees.
    std::for_each(t->begin(), t->end(), [&]( const Node& n ) {
            if ( done.count(n.name) )
            {
                return;
            }
            if ( n.target_type() == typeid(Tree) )
            {
                Tree subtree = *n.target<Tree>();
                if ( !descriptor::catalog.count( subtree.type ) )
                {
                    throw polysync::error( "missing type descriptor" )
                        << exception::type( subtree.type );
                }
                const descriptor::Type& subtype = descriptor::catalog.at( subtree.type );
                BOOST_LOG_SEV( log, severity::debug1 ) << "recursing subtree " << subtree.type;
                return encode( subtree, subtype );
            }
            if ( n.target_type() == typeid(Bytes) )
            {
                const Bytes& raw = *n.target<Bytes>();
                BOOST_LOG_SEV( log, severity::debug2 ) << "writing " << raw.size()
                    << " raw bytes to offset " << stream.tellp();
                stream.write( (char *)raw.data(), raw.size() );
                return;
            }
            BOOST_LOG_SEV( log, severity::warn )
                << "field \"" << n.name << "\" not serialized due to lack of description";
            });
}

} // namespace polysync
