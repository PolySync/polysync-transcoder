#include <polysync/size.hpp>

#include <polysync/plog/iterator.hpp>
#include <polysync/plog/decoder.hpp>

namespace polysync { namespace plog {

using logging::severity;

iterator::iterator( std::streamoff pos ) : pos( pos )
{
}

iterator::iterator( decoder* s, std::streamoff pos ) : stream( s ), pos( pos )
{
    static const std::streamoff recordSize = size<ps_log_record>::value();

    BOOST_LOG_SEV( stream->log, severity::debug1 )
        << "decoding \"plog::ps_log_record\" at " << pos << ":" << pos + recordSize;

    header = stream->decode<ps_log_record>( pos );
}

ps_log_record iterator::operator*()
{
    if ( !header )
        throw polysync::error( "dereferenced invalid iterator" );
    return header.get();
}

iterator& iterator::operator++()
{
    static const std::streamoff recordSize = size<ps_log_record>::value();

    // Advance the iterator's position to the beginning of the next record and
    // read the new header.
    pos += recordSize + header->size;
    try
    {
        stream->decode( *header, pos );
        BOOST_LOG_SEV( stream->log, severity::debug1 )
            << "decoding \"plog::ps_log_record\" between " << pos << ":" << pos + recordSize;

    } catch ( polysync::error )
    {
        header = boost::none;
    }

    return *this;
}

}} // namespace polysync::plog
