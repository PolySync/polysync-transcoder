#include <iostream>
#include <algorithm>
#include <regex>

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>

#include <polysync/plog/core.hpp>
#include <polysync/plog/decoder.hpp>
#include <polysync/plugin.hpp>
#include <polysync/detector.hpp>
#include <polysync/logging.hpp>
#include <polysync/exception.hpp>
#include <polysync/console.hpp>
#include <polysync/print_hana.hpp>

namespace po = boost::program_options;
namespace fs = boost::filesystem;
namespace plog = polysync::plog;
namespace ps = polysync;

using ps::logging::severity;
using ps::logging::logger;

namespace polysync {

namespace toml { extern po::options_description options( const fs::path& exe ); }
namespace toml { extern void load(const po::variables_map&);  }

} // namespace polysync

namespace std {

// boost::program_options likes to print the arguments, so here we teach it how
// to print out a vector of fs::path.
std::ostream& operator<<(std::ostream& os, const std::vector<fs::path>& paths) {
    std::for_each( paths.begin(), paths.end(), [&os](const fs::path& p) { os << p << " "; } );
    return os;
}

} // namespace std


int catch_main( int ac, char* av[] ) {

    logger log( "transcode" );

    using polysync::format;

    // By default, use a fancy formatter.  This can change below.
    format = std::make_shared< polysync::formatter::fancy >();

    // Define a rather sophisticated command line parse that supports
    // subcommands (rather like git) where each subcommand is a supported
    // output driver (like CSV, HDF5, or SciDB).  Originally described by
    // http://stackoverflow.com/questions/15541498/how-to-implement-subcommands-using-boost-program-options
    
    // compute reasonable guess about plugin and share locations.
    char exebuf[1024];
    size_t exelen = readlink( "/proc/self/exe", exebuf, 1024 ); // This is my own path, compliments of /proc.
    fs::path exe( std::string( exebuf, exelen ) );

    po::options_description general_opt( "General Options" );
    general_opt.add_options()
        ( "help,h", "print this help message" )
        ( "verbose,v", po::value<std::string>()->default_value("info"), "debug level" )
        ( "plain,p", "remove color from console formatting" )
        ;

    po::options_description positional_opt;
    positional_opt.add_options()
        ( "path", po::value<std::vector<fs::path>>(), "plog input files" )
        ( "encoder", po::value<std::string>()->default_value("list"), "Output encoder" )
        ( "subargs", po::value<std::vector<std::string>>(), "Encoder arguments" )
        ;

    po::positional_options_description posdesc;
    posdesc.add( "path", 1 ).add( "encoder", 1 ).add( "subargs", -1 );

    po::options_description cmdline, helpline;  // These are the same except for positional_opt.
    po::options_description toml_opt = polysync::toml::options( exe );
    po::options_description plugin_opt = polysync::plugin::options( exe );
    cmdline.add( general_opt ).add( toml_opt ).add( plugin_opt ).add( positional_opt );
    helpline.add( general_opt ).add( toml_opt ).add( plugin_opt );

    // Do the first of two parses.  This first one is required to discover the
    // plugin path and the encoder name.
    po::variables_map vm;
    po::parsed_options parse = po::command_line_parser( ac, av )
        .options(cmdline)
        .positional(posdesc)
        .allow_unregistered()
        .run();
    po::store( parse, vm );
    po::notify( vm );

    // Set the debug and console format options right away
    ps::logging::set_level( vm["verbose"].as<std::string>() );

    if (vm.count("plain"))
        format = std::make_shared<polysync::formatter::plain>();

    polysync::toml::load( vm );

    po::options_description filter_opts = polysync::filter::load( vm, exe );
    cmdline.add( filter_opts );
    helpline.add( filter_opts );

    po::options_description encode_opts = polysync::encode::load( vm, exe );
    cmdline.add( encode_opts );
    helpline.add( encode_opts );

    if ( vm.count("help") ) {
        std::cout << format->header( "PolySync Transcoder" ) << std::endl << std::endl;
        std::cout << "Usage:" << std::endl;
        std::cout << "\ttranscode [options] <input-file> <encoder> [encoder-options]" << std::endl;
        std::cout << helpline << std::endl << std::endl;
        return ps::status::ok;
    }

    if ( !vm.count("path") ) 
        throw ps::error( "no input files" ) << ps::status::bad_input;

    std::string encoder = vm["encoder"].as<std::string>();
    if ( !ps::encode::map.count( encoder ) ) {
        std::cerr << "error: unknown output \"" << encoder << "\"" << std::endl;
        exit( ps::status::no_plugin );
    }

    std::vector<std::string> opts = po::collect_unrecognized( parse.options, po::include_positional );
    if ( !opts.empty() ) {
        opts.erase( opts.begin() );

        po::options_description encoder_opts = ps::encode::map.at( encoder )->options();

        // Parse again, with now with encoder options
        po::store( po::command_line_parser( opts ).options( encoder_opts ).run(), vm );
        po::notify(vm);
    }

    std::function<bool ( const plog::log_record& )> filter = []( const plog::log_record& ) { return true; };

    ps::descriptor::catalog.emplace( "log_record", ps::descriptor::describe<plog::log_record>::type() );
    ps::descriptor::catalog.emplace( "msg_header", ps::descriptor::describe<plog::msg_header>::type() );

    // Set observer patterns, with the subject being the iterated plog readers
    // and the iterated records from each.  The observers being callbacks
    // requested in the command line arguments
    ps::encode::visitor visit;

    // PLogs key each message type by a number, but the number is generally
    // different in every plog.  Constant names are provided in the header in
    // the type_support field; stash them so we can map the (random) numbers to
    // (useful) static strings.  Most if not every plugin needs these, so just
    // add it globally here.
    visit.type_support.connect( []( plog::type_support t ) { 
            // Don't bother registering detectors for types if do not have the descriptor.
            if ( ps::descriptor::catalog.count(t.name) )
                 ps::detector::catalog.push_back(ps::detector::type { "msg_header", 
                    { { "type", t.type } }, 
                    t.name } );
            plog::type_support_map.emplace( t.type, t.name ); 
            } );

    ps::encode::map.at( encoder )->connect( vm, visit );

   
    // The observers are finally all set up.  Here, we finally do the computation!
    // Double iterate over files from the command line, and records in each file.
    for ( fs::path path: vm["path"].as<std::vector<fs::path>>() ) 
    {
        try {
            std::ifstream st( path.c_str(), std::ifstream::binary );
            if ( !st )
                throw polysync::error( "cannot open file" );

            // Construct the next reader in the file list
            plog::decoder decoder( st );

            visit.open( decoder );

            plog::log_header head;

            decoder.decode( head );
            visit.log_header( head );
            for ( const plog::type_support& type: head.type_supports )
                visit.type_support( type );
            for ( const plog::log_record& rec: decoder ) {
                if ( filter(rec) ) {
                    BOOST_LOG_SEV( log, severity::verbose ) << rec;
                    polysync::node top( "log_record", decoder.deep(rec) );
                    visit.record(top);
                }
            }
            visit.cleanup( decoder );
        } catch ( polysync::error& e ) {
            e << ps::exception::path( path.c_str() );
            e << polysync::status::bad_input;
            throw;
        }
    }

    // We seemed to have survive the run!
    return ps::status::ok;
}

int main( int ac, char* av[] ) {
    try {
        return catch_main( ac, av );
    } catch ( const ps::error& e ) {
        // Print any context provided by the exception.
        std::cerr << polysync::format->error( "Transcoder abort: " ) << e;
        if ( const ps::status* stat = boost::get_error_info<ps::exception::status>(e) ) 
            exit( *stat );
    } catch ( const po::error& e ) {
        std::cerr << polysync::format->error( "Transcoder abort: " ) << e.what() << std::endl;
        return polysync::status::bad_argument;
    }
}

