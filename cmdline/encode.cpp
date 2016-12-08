#include <regex>

// Use boost::dll and boost::filesystem to manage plugins.  Among other
// benefits, it eases a Windows port.
#include <boost/dll/import.hpp>
#include <boost/dll/shared_library.hpp>
#include <boost/dll/runtime_symbol_info.hpp> // for program_location()
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>

#include <polysync/exception.hpp>
#include <polysync/logging.hpp>
#include <polysync/plugin.hpp>

namespace polysync { namespace encode {

namespace po = boost::program_options;
namespace fs = boost::filesystem;
namespace dll = boost::dll;

using logging::severity;
using logging::logger;

static logger log( "encode" );

std::map< std::string, boost::shared_ptr<encode::plugin> > map;

po::options_description load( const std::vector<fs::path>& plugpath ) {

    po::options_description options( "Encoder Plugins:" );

    // We have some hard linked plugins.  This makes important ones and that
    // have no extra dependencies always available, even if the plugin path is
    // misconfigured.  It also makes the query options appear first in the help
    // screen, also useful.
    dll::shared_library self( dll::program_location() );
    for ( std::string plugname: { "list", "dump" } ) {
        auto plugin_factory = 
            self.get_alias<boost::shared_ptr<encode::plugin>()>( plugname + "_plugin" );
        auto plugin = plugin_factory();
        options.add( plugin->options() );
        encode::map.emplace( plugname, plugin );
    }

    // Iterate the plugin path, and for each path load every valid entry in
    // that path.  Add options to parser.
    for ( fs::path plugdir: plugpath ) {

        plugdir = plugdir / "encode";

        if ( !fs::exists( plugdir ) ) {
            BOOST_LOG_SEV( log, severity::debug1 ) 
                << "skipping non-existing plugin path " << plugdir;
            continue;
        }

        BOOST_LOG_SEV( log, severity::debug1 ) 
            << "searching " << plugdir << " for encoder plugins";
        static std::regex is_plugin( R"(.*encode\.(.+)\.so)" );
        for ( fs::directory_entry& lib: fs::directory_iterator(plugdir) ) {
            std::cmatch match;
            std::regex_match( lib.path().string().c_str(), match, is_plugin );
            if ( match.size() ) {
                std::string plugname = match[1];

                if ( encode::map.count(plugname) ) {
                    BOOST_LOG_SEV( log, severity::debug2 ) 
                        << "encoder \"" << plugname << "\"already loaded";
                    continue;
                }

                try {
                    boost::shared_ptr<encode::plugin> plugin = 
                        dll::import<encode::plugin>( lib.path(), "encoder" );
                    BOOST_LOG_SEV( log, severity::debug1 ) 
                        << "loaded encoder from " << lib.path();
                    po::options_description opt = plugin->options();
                    options.add( opt );
                    encode::map.emplace( plugname, plugin );
                } catch ( std::runtime_error& ) {
                    BOOST_LOG_SEV( log, severity::debug2 ) 
                        << lib.path() << " provides no encoder; symbols not found";
                }
            } else {
                BOOST_LOG_SEV( log, severity::debug2 ) 
                    << lib.path() << " provides no encoder; filename mismatch";
            }
        }
    }

    return options;
}

}} // namespace polysync::encode
