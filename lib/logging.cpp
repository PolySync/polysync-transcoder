#include <iostream>
#include <regex>

// Would prefer std::shared_ptr<>, but boost::log needs an API update for that
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/smart_ptr/make_shared_object.hpp>
#include <boost/core/null_deleter.hpp>

#include <boost/log/core.hpp>
#include <boost/log/sinks/text_ostream_backend.hpp>
#include <boost/log/sinks/sync_frontend.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/attributes.hpp>

#include <polysync/logging.hpp>
#include <polysync/exception.hpp>
#include <polysync/console.hpp>

namespace polysync {

std::shared_ptr<formatter::interface> format { new formatter::fancy() };

namespace logging {

namespace expr = boost::log::expressions;

BOOST_LOG_ATTRIBUTE_KEYWORD( severity_attr, "Severity", logging::severity );
BOOST_LOG_ATTRIBUTE_KEYWORD( channel_attr, "Channel", std::string );

std::string to_string(severity s)
{
    switch (s)
    {
        case severity::error: return format->error("[err]");
        case severity::warn: return format->warn("[warn]");
        case severity::info: return format->info("[info]");
        case severity::verbose: return format->verbose("[verbose]");
        case severity::debug1: return format->debug("[debug1]");
        case severity::debug2: return format->debug("[debug2]");
    };
}

std::map< std::string, severity > severityMap =
{
    { "debug1", severity::debug1 },
    { "debug2", severity::debug2 },
    { "info", severity::info },
    { "verbose", severity::verbose },
    { "warn", severity::warn },
    { "error", severity::error },
};

static severity defaultSeverity;
static std::map< std::string, severity > channelSeverity;

void setLevels( const std::vector<std::string>& settings )
{
    for ( std::string cmdline: settings )
    {
        std::smatch match;
        if ( !std::regex_match( cmdline, match, std::regex( R"(([A-z0-9]+):?([A-z0-9]+)?)" ) ) )
        {
            throw error( "malformed loglevel argument \"" + cmdline + "\"" );
        }
        std::string level = match[1];
        if ( !severityMap.count(level) )
        {
            throw polysync::error( "unknown debug level \"" + level + "\"" );
        }
        std::string channel = match[2];
        if ( !channel.empty() )
        {
            channelSeverity.emplace( channel, severityMap.at(level) );
        }
        else
        {
            defaultSeverity = severityMap.at(level);
        }
    }
}

struct init_logger
{
    init_logger()
    {
        namespace log = boost::log;
        using backend_type = log::sinks::text_ostream_backend;
        boost::shared_ptr<backend_type> backend = boost::make_shared<backend_type>();

        boost::shared_ptr<std::ostream> stream(&std::clog, boost::null_deleter());
        backend->add_stream( stream );
        backend->auto_flush( true );

        using sink_type = log::sinks::synchronous_sink<backend_type>;
        boost::shared_ptr<sink_type> sink( new sink_type(backend) );

        sink->set_formatter ( [] ( const log::record_view& rec, log::formatting_ostream& strm )
                {
                    strm
                        << format->channel(log::extract<std::string>( "Channel", rec ).get<std::string>())
                        << to_string( log::extract<severity>( "Severity", rec ).get<severity>() )
                        << ": " << rec[expr::smessage];
                } );

        auto core = boost::log::core::get();

        core->set_filter( []( const log::attribute_value_set& record )
                {
                    severity sev = log::extract<severity>( "Severity", record ).get<severity>();
                    std::string channel = log::extract<std::string>( "Channel", record ).get<std::string>();

                    auto it = channelSeverity.find( channel );
                    if ( it == channelSeverity.end() )
                    {
                        return sev <= defaultSeverity;
                    }
                    return sev <= it->second;
                } );

        core->add_sink(sink);
    }
};

static init_logger init;

}} // namespace polysync::logging

