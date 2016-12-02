#include <polysync/logging.hpp>
#include <polysync/exception.hpp>
#include <polysync/console.hpp>

#include <boost/log/core.hpp>
#include <boost/log/sinks/text_ostream_backend.hpp>
#include <boost/log/sinks/sync_frontend.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/attributes.hpp>
#include <boost/core/null_deleter.hpp>

// Would prefer std::shared_ptr<>, but boost::log needs an API update for that
#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/smart_ptr/make_shared_object.hpp>

#include <iostream>

namespace polysync { 

std::shared_ptr<formatter::interface> format { new formatter::fancy() };

namespace logging {

namespace expr = boost::log::expressions;

BOOST_LOG_ATTRIBUTE_KEYWORD(severity_attr, "Severity", logging::severity);

std::string to_string(severity s)
{
    switch (s) {
        case severity::error: return format->error("[err]");
        case severity::warn: return format->warn("[warn]"); 
        case severity::info: return format->info("[info]"); 
        case severity::verbose: return format->verbose("[verbose]");
        case severity::debug1: return format->debug("[debug1]"); 
        case severity::debug2: return format->debug("[debug2]");
    };
}

std::map<std::string, severity> severity_map = {
    { "debug1", severity::debug1 },
    { "debug2", severity::debug2 },
    { "info", severity::info },
    { "verbose", severity::verbose },
    { "warn", severity::warn },
    { "error", severity::error },

};

void set_level(const std::string& level) {
    if (!severity_map.count(level))
        throw polysync::error("unknown debug level \"" + level + "\"");

    boost::log::core::get()->set_filter(severity_attr <= severity_map.at(level));
};

struct init_logger {
    init_logger() {
        using backend_type = boost::log::sinks::text_ostream_backend;
        boost::shared_ptr<backend_type> backend = boost::make_shared<backend_type>();

        boost::shared_ptr<std::ostream> stream(&std::clog, boost::null_deleter());
        backend->add_stream(stream);
        backend->auto_flush(true);

        using sink_type = boost::log::sinks::synchronous_sink<backend_type>;
        boost::shared_ptr<sink_type> sink(new sink_type(backend));

        sink->set_formatter ([](const boost::log::record_view& rec, boost::log::formatting_ostream& strm) {
                 strm
                 << format->channel(boost::log::extract<std::string>("Channel", rec).get<std::string>())
                 << to_string(boost::log::extract<severity>("Severity", rec).get<severity>())
                 << ": " << rec[expr::smessage];
                 }); 

        auto core = boost::log::core::get();
        core->add_sink(sink);
    }
};

static init_logger init;

}} // namespace polysync::logging

