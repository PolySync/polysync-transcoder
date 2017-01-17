#pragma once

// #define BOOST_LOG_DYN_LINK 1

#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/sources/severity_channel_logger.hpp>

namespace polysync { namespace logging {

enum struct severity { error, warn, info, verbose, debug1, debug2 };

using sourceType = boost::log::sources::severity_channel_logger_mt<severity, std::string>;

struct logger : sourceType
{
    logger( const std::string& channel ) : sourceType( boost::log::keywords::channel = channel )
    {
    }
};

void setLevel( const std::string& );

}} // namespace polysync::logging
