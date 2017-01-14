#pragma once

#include <iostream>

#include <boost/optional.hpp>

#include <polysync/size.hpp>
#include <polysync/plog/core.hpp>

namespace polysync {

using logging::severity;

template <typename Header>
struct Sequencer;


// Define an STL compatible iterator to traverse plog files
template <typename Header>
struct Iterator {

    Iterator( std::streamoff pos ) : pos( pos )
    {
    }

    Iterator( Sequencer<Header>* s, std::streamoff pos ) : stream( s ), pos( pos )
    {
        static const std::streamoff recordSize = size<Header>::value();

        BOOST_LOG_SEV( stream->log, severity::debug1 )
            << "decoding header at " << pos << ":" << pos + recordSize;

        header = stream->template decode<Header>( pos );
    }


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

    Header operator*() // read and return the payload
    {
        if ( !header )
        {
            throw polysync::error( "dereferenced invalid iterator" );
        }
        return header.get();
    }

    Iterator<Header>& operator++() // skip to the next record
    {
        static const std::streamoff recordSize = size<Header>::value();

        // Advance the iterator's position to the beginning of the next record and
        // read the new header.
        pos += recordSize + header->size;
        try
        {
            stream->decode( *header, pos );
            BOOST_LOG_SEV( stream->log, severity::debug1 )
                << "decoding header between " << pos << ":" << pos + recordSize;
        }
        catch ( polysync::error )
        {
            header = boost::none;
        }

        return *this;
    }

};

} // namespace polysync
