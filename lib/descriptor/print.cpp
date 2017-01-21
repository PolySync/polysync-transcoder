#include <polysync/descriptor/print.hpp>
#include <polysync/descriptor/lex.hpp>
#include <polysync/console.hpp>

namespace polysync { namespace descriptor {

std::ostream& operator<<( std::ostream& os, const Field& field )
{
    return os << format->fieldname( field.name + ": " )
              << format->value( lex(field.type) );
}

std::ostream& operator<<( std::ostream& os, const Type& desc )
{
    os << format->type( desc.name + ": { " );
    std::for_each( desc.begin(), desc.end(), [&os](auto field) { os << field << ", "; });
    os.seekp( -2, std::ios_base::end ); // remove that last comma
    return os << format->type(" }");
}

std::ostream& operator<<(std::ostream& os, const BitField& bitField )
{
    os << format->begin_block( "bitfield" );
    for ( auto pair: bitField.fields )
    {
        os << format->begin_item() << lex(pair) << format->end_item();
    }
    return os << format->end_block();
}

}} // namespace polysync::descriptor
