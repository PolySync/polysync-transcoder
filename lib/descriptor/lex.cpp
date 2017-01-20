#include <polysync/descriptor/lex.hpp>
#include <polysync/descriptor/catalog.hpp>

namespace polysync { namespace descriptor {

std::string lex::operator()( std::type_index idx ) const
{
    return terminalTypeMap.at(idx).name;
}

std::string lex::operator()( Bit idx ) const
{
    return idx.name;
}

std::string lex::operator()( Nested idx ) const
{
    return idx.name;
}

std::string lex::operator()( Skip idx ) const
{
    return "skip-" + std::to_string(idx.order) + "(" + std::to_string(idx.size) + ")";
}

std::string lex::operator()( BitSkip idx ) const
{
    return "bitskip-" + std::to_string(idx.order) + "(" + std::to_string(idx.size) + ")";
}

std::string lex::operator()( BitField idx ) const
{
    return "BitField";
}

std::string lex::operator()( Array idx ) const
{
    return "array<" + eggs::variants::apply(*this, idx.type) + ">("
        + eggs::variants::apply(*this, idx.size) + ")";
}

std::string lex::operator()( std::string s ) const
{
    return s;
}

std::string lex::operator()( size_t s ) const
{
    return std::to_string(s);
}

}} // namespace polysync::descriptor
