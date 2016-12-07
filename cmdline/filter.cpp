#include <regex>
#include <cstdlib> // for getenv()

#include <boost/dll/import.hpp>
#include <boost/dll/shared_library.hpp>
#include <boost/dll/runtime_symbol_info.hpp> // for program_location()
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>

#include <polysync/exception.hpp>
#include <polysync/logging.hpp>
#include <polysync/plugin.hpp>

namespace polysync { namespace filter {

namespace po = boost::program_options;
namespace fs = boost::filesystem;
namespace dll = boost::dll;

using logging::severity;
using logging::logger;

static logger log( "filter" );
std::map< std::string, boost::shared_ptr<filter::plugin> > map;

po::options_description load() {

    // We have some hard linked plugins.  This makes important ones and that
    // have no extra dependencies always available, even if the plugin path is
    // misconfigured.  It also makes the query options appear first in the help
    // screen, also useful.
    dll::shared_library self( dll::program_location() );
    for ( std::string plugname: { "slice" } ) {
        auto factory = self.get_alias<boost::shared_ptr<filter::plugin>()>( plugname + "_plugin" );
        filter::map.emplace( plugname, factory() );
        BOOST_LOG_SEV( log, severity::debug1 ) << "\"" << plugname << "\" found: hard linked";
    }


    // Iterate the plugin path, and for each path load every valid entry in
    // that path.  Add options to parser.
    std::vector<std::string> libdirs;
    char* libenv = std::getenv("POLYSYNC_TRANSCODER_LIB");
    boost::split( libdirs, libenv, boost::is_any_of(";") );
    for ( fs::path plugdir: libdirs ) { // cmdline_args["plugdir"].as<std::vector<fs::path>>() ) {

        plugdir = plugdir / "filter";

        if ( !fs::exists( plugdir ) ) {
            BOOST_LOG_SEV( log, severity::debug1 ) << "skipping non-existing plugin path " << plugdir;
            continue;
        }

        BOOST_LOG_SEV( log, severity::debug1 ) << "searching " << plugdir << " for filter plugins";
        static std::regex is_plugin( R"(.*filter\.(.+)\.so)" );
        for ( fs::directory_entry& lib: fs::directory_iterator(plugdir) ) {
            std::cmatch match;
            std::regex_match( lib.path().string().c_str(), match, is_plugin );
            if ( match.size() ) {
                std::string plugname = match[1];

                if ( encode::map.count(plugname) ) {
                    BOOST_LOG_SEV( log, severity::debug2 ) << "filter \"" << plugname << "\"already loaded";
                    continue;
                }

                try {
                    boost::shared_ptr<filter::plugin> plugin = 
                        dll::import<filter::plugin>( lib.path(), "filter" );
                    BOOST_LOG_SEV( log, severity::debug1 ) << "loaded filter from " << lib.path();
                    filter::map.emplace( plugname, plugin );
                } catch ( std::runtime_error& ) {
                    BOOST_LOG_SEV( log, severity::debug2 ) 
                        << lib.path() << " provides no filter; symbols not found";
                }
            } else {
                BOOST_LOG_SEV( log, severity::debug2 ) 
                    << lib.path() << " provides no filter; filename mismatch";
            }
        }
    }

    po::options_description options( "Filtering Plugins:" );
    for ( auto pair: filter::map )
        options.add( pair.second->options() );
    return options;
    }

}} // namespace polysync::filter


