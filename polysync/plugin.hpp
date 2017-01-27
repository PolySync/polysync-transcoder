#pragma once

#include <boost/config.hpp>  // for BOOST_SYMBOL_EXPORT
#include <boost/program_options.hpp>
#include <boost/signals2/signal.hpp>
#include <boost/dll/alias.hpp>
#include <boost/filesystem.hpp>

#include <polysync/tree.hpp>
#include <polysync/decoder/decoder.hpp>

namespace po = boost::program_options;
namespace fs = boost::filesystem;

namespace polysync { namespace encode {

extern po::options_description options();

// Visitation points.
struct Visitor
{
    template <typename T>
    using callback = boost::signals2::signal<void (T)>;

    using Seq = Sequencer<plog::ps_log_record>;

    callback< const Seq& > open; // New plog file was opened.
    callback< plog::ps_log_header& > log_header; // New plog file was opened and log_header read.
    callback< const Node& > record; // Response to each log_record
    callback< const Seq& > cleanup; // Decoder is destructed
    callback< const plog::ps_type_support& > type_support; // Type support name/number association

};

struct plugin {
    // Every plugin must provide any special command line options it needs.
    virtual po::options_description options() const = 0;

    // Every plugin must register the callbacks it needs.
    virtual void connect( const po::variables_map&, Visitor& ) = 0;
};

extern po::options_description load( const std::vector<fs::path>& );
extern std::map< std::string, boost::shared_ptr<encode::plugin> > map;

} // namespace encode

namespace filter {

using type = std::function<bool (const plog::ps_log_record&)>;

struct plugin {
    virtual po::options_description options() const = 0;
    virtual type predicate( const po::variables_map& ) const = 0;
};

extern po::options_description load( const std::vector<fs::path>& );
extern std::map< std::string, boost::shared_ptr<filter::plugin> > map;

} // namespace filter

namespace decode {

using type = void;  // Hmm, what should this look like?

struct plugin {
    virtual po::options_description options() const = 0;
    virtual type decoder() const = 0;
};

} // namespace decode

} // namespace polysync
