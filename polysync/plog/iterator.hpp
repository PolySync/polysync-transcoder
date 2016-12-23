#pragma once

#include <iostream>

#include <boost/optional.hpp>

#include <polysync/plog/core.hpp>

namespace polysync { namespace plog {

struct decoder;

// Define an STL compatible iterator to traverse plog files
struct iterator {

    iterator( std::streamoff );
    iterator( decoder*, std::streamoff );

    decoder* stream { nullptr };
    std::streamoff pos; // file offset from std::ios_base::beg, pointing to next record
    boost::optional<ps_log_record> header; // unset on failed decode of ps_log_record

    bool operator!=( const iterator& other ) const
    {
        return pos < other.pos;
    }

    bool operator==( const iterator& other ) const
    {
        return pos == other.pos;
    }

    ps_log_record operator*(); // read and return the payload
    iterator& operator++(); // skip to the next record
};

}} // namespace polysync::plog
