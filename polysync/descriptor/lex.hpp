#pragma once

#include <eggs/variant.hpp>

#include <polysync/descriptor/type.hpp>

namespace polysync { namespace descriptor {

// Pretty printer for descriptors
struct lex : std::string
{
    template <typename T>
    lex( const T& v );

    std::string operator()( std::type_index ) const;
    std::string operator()( Nested ) const;
    std::string operator()( Skip ) const;
    std::string operator()( Array ) const;
    std::string operator()( std::string ) const;
    std::string operator()( size_t ) const;

};

template <typename T>
lex::lex( const T& v ) : std::string( eggs::variants::apply(*this, v) ) {}

}} // namespace polysync::descriptor


