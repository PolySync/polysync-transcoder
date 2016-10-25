#pragma once

#include <polysync/transcode/decoder.hpp>

#include <boost/config.hpp>  // for BOOST_SYMBOL_EXPORT
#include <boost/program_options.hpp>
#include <boost/signals2/signal.hpp>
#include <boost/dll/alias.hpp>

namespace polysync { namespace transcode {

// Visitation points.
struct visitor {
    template <typename T> 
    using callback = boost::signals2::signal<void (T)>;

    callback<plog::decoder&> decoder; // New plog file was opened.
    callback<const plog::log_record&> record; // Response to each log_record
    callback<const plog::decoder&> cleanup; // Decoder is destructed
    callback<const plog::type_support&> type_support; // Type support name/number association
};

namespace po = boost::program_options;

struct plugin {
    // Every plugin must provide any special command line options it needs.
    virtual po::options_description options() const = 0;

    // Every plugin must register the callbacks it needs.
    virtual void connect(const po::variables_map&, transcode::visitor&) const = 0;
};

}} // namespace polysync::transcode
