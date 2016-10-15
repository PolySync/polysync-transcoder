#pragma once

#include <polysync/transcode/reader.hpp>

#include <boost/config.hpp>  // for BOOST_SYMBOL_EXPORT
#include <boost/program_options.hpp>
#include <boost/signals2/signal.hpp>
#include <boost/dll/alias.hpp>

namespace polysync { namespace transcode {

struct callback {
    boost::signals2::signal<void (plog::reader&)> reader;
    boost::signals2::signal<void (const plog::log_record&)> record;
    boost::signals2::signal<void (const plog::reader&)> cleanup;
    boost::signals2::signal<void (const plog::type_support&)> type_support;
};

namespace po = boost::program_options;

struct plugin {
    virtual po::options_description options() const = 0;
    virtual void connect(const po::variables_map&, callback&) const = 0;
};

}} // namespace polysync::transcode
