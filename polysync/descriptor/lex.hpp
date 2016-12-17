#pragma once

#include <polysync/descriptor.hpp>

namespace polysync { namespace descriptor {

struct lex : std::string {

    lex( const field::variant& v );
    std::string operator()( std::type_index ) const;
    std::string operator()( nested ) const;
    std::string operator()( skip ) const;
    std::string operator()( array ) const;
    std::string operator()( std::string ) const;
    std::string operator()( size_t ) const;

};

lex::lex(const field::variant& v) : std::string(eggs::variants::apply(*this, v)) {}

}} // namespace polysync::descriptor


