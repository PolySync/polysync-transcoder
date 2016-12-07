#pragma once

#include <boost/config.hpp>  // for BOOST_SYMBOL_EXPORT
#include <boost/program_options.hpp>
#include <boost/signals2/signal.hpp>
#include <boost/dll/alias.hpp>
#include <boost/filesystem.hpp>

#include <polysync/tree.hpp>
#include <polysync/plog/decoder.hpp>

namespace po = boost::program_options;
namespace fs = boost::filesystem;

namespace polysync { 

namespace encode {

extern po::options_description options();

// Visitation points.
struct visitor {

    template <typename T> 
    using callback = boost::signals2::signal<void (T)>;

    callback< plog::decoder& > open; // New plog file was opened.
    callback< plog::log_header& > log_header; // New plog file was opened and log_header read.
    callback< const node& > record; // Response to each log_record
    callback< const plog::decoder& > cleanup; // Decoder is destructed
    callback< const plog::type_support& > type_support; // Type support name/number association

};

struct plugin {
    // Every plugin must provide any special command line options it needs.
    virtual po::options_description options() const = 0;

    // Every plugin must register the callbacks it needs.
    virtual void connect( const po::variables_map&, visitor& ) = 0;
};

extern po::options_description load();
extern std::map< std::string, boost::shared_ptr<encode::plugin> > map;

} // namespace encode

namespace filter {

using type = std::function<bool (const plog::log_record&)>;

struct plugin {
    virtual po::options_description options() const = 0;
    virtual type predicate( const po::variables_map& ) const = 0;  
};

extern po::options_description load(); 
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
