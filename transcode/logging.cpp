#include <polysync/transcode/logging.hpp>
#include <polysync/transcode/console.hpp>

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

namespace polysync { namespace logging {

namespace expr = boost::log::expressions;

BOOST_LOG_ATTRIBUTE_KEYWORD(severity_attr, "Severity", logging::severity);

static struct console::color console;

std::ostream& operator<< (std::ostream& os, severity s)
{
    static std::map<severity, std::string> pretty = {
        { severity::error, console.red + std::string("err") + console.normal },
        { severity::warn, console.magenta + std::string("warn") + console.normal },
        { severity::info, console.yellow + std::string("info") + console.normal },
        { severity::verbose, console.yellow + std::string("verbose") + console.normal },
        { severity::debug1, console.white + std::string("debug1") + console.normal },
        { severity::debug2, console.white + std::string("debug2") + console.normal },
    };

    return os << pretty.at(s);
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
        throw std::runtime_error("unknown debug level \"" + level + "\"");

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

        sink->set_formatter (
                 expr::stream
                 << console.green << console.bold << expr::attr<std::string>("Channel") << console.normal
                 << "[" << severity_attr << "]"
                 << ": " << expr::smessage
                 );

        auto core = boost::log::core::get();
        core->add_sink(sink);
    }
};

static init_logger init;

}} // namespace polysync::logging

