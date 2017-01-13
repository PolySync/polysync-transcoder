#pragma once

#include <iostream>

#include <boost/optional.hpp>

#include <polysync/plog/core.hpp>

namespace polysync {

template <typename Header>
struct Sequencer;

// Define an STL compatible iterator to traverse plog files
template <typename Header>
struct Iterator {

    iterator( std::streamoff );
    iterator( Sequencer<Header>*, std::streamoff );

    Sequencer<Header>* stream { nullptr };
    std::streamoff pos; // file offset from std::ios_base::beg, pointing to next record
    boost::optional<Header> header; // unset on failed decode of ps_log_record

    bool operator!=( const Iterator<Header>& other ) const
    {
        return pos < other.pos;
    }

    bool operator==( const Iterator<Header>& other ) const
    {
        return pos == other.pos;
    }

    Header operator*(); // read and return the payload
    Iterator<Header>& operator++(); // skip to the next record
};

} // namespace polysync
