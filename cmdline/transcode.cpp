#include <iostream>
#include <algorithm>
#include <regex>
#include <cstdlib>

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>

#include <polysync/plog/core.hpp>
#include <polysync/plog/decoder.hpp>
#include <polysync/plugin.hpp>
#include <polysync/toml.hpp>
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

namespace std {

// boost::program_options likes to print the arguments, so here we teach it how
// to print out a vector of fs::path.
std::ostream& operator<<(std::ostream& os, const std::vector<fs::path>& paths) {
    std::for_each( paths.begin(), paths.end(), [&os](const fs::path& p)
            { os << p << " "; } );
    return os;
}

} // namespace std

// The command line parse is complicated because the plugins are allowed to
// add options, and finding the plugins themselves require an opportunity
// to set paths to their location before loading.
//
// There are three stages:
// 1) Bootstrap: Configure the console spew because this will affect the second
//    stage.  Also, accept more plugin paths.
// 2) Input/Output: Parse the positional arguments (input and encoder) to
//    identify the encoder's name.  Filters are also parsed in this stage.  Filter
//    options cannot be duplicated; take care in the filter plugins that the
//    option names are not reserved by some other plugin.
// 3) Encoder: The final parse enables only the options from the chosen encoder.
//    This allows duplicate options between different encoder plugins, which will
//    be common (like output file name).
//
// The encoders are handled similarly to subcommands (like git does).  This
// pattern was originally described for boost::program_options by
// http://stackoverflow.com/questions/15541498/how-to-implement-subcommands-using-boost-program-options

class cmdline_parser {

public:

    void bootstrap( int ac, char* av[] );
    po::parsed_options configure_input_output( int ac, char* av[] );
    void configure_encoder( po::parsed_options& );
    void print_usage();

    // All the stages accumulate results into one container of arguments,
    // called cmdline_args.
    po::variables_map cmdline_args;

protected:

    logger log { "cmdline" };

    po::options_description bootstrap_options { "Bootstrap Options" };
    po::options_description toml_options { "Type Description Options" };
    po::options_description filter_options { "Filter Plugins" };
    po::options_description encode_options { "Encoder Plugins" };

};

void cmdline_parser::print_usage() {

    if ( cmdline_args.count("help") ) {

        po::options_description helpline;
        helpline.add( bootstrap_options )
                // .add( toml_options ) // These are currently empty
                .add( filter_options )
                .add( encode_options );

        std::cout << polysync::format->header( "PolySync Transcoder" )
                  << std::endl << std::endl;
        std::cout << "Usage:" << std::endl;
        std::cout << "\ttranscode [options] <input-file> <encoder> "
                  << "[encoder-options]" << std::endl;
        std::cout << helpline << std::endl << std::endl;

        exit( ps::status::ok );
    }
}

void cmdline_parser::bootstrap( int ac, char* av[] ) {

    // Stage 1: Bootstrap options to configure plugin paths and debug level

    bootstrap_options.add_options()
        ( "help,h", "print this help message" )
        ( "verbose,v", po::value<std::string>()->default_value("info"),
          "debug level [ info verbose debug1 debug2 ]" )
        ( "plain,p", "remove color from console formatting" )
        ;

    po::parsed_options parse_results = po::command_line_parser( ac, av )
        .options(bootstrap_options)
        .allow_unregistered() // Pass through everything except bootstrap options
        .run();
    po::store( parse_results, cmdline_args );
    po::notify( cmdline_args );

    // Set the debug and console format options right away so log messages in
    // the subsequent plugin loaders are processed correctly.
    ps::logging::set_level( cmdline_args["verbose"].as<std::string>() );

    if (cmdline_args.count("plain"))
        polysync::format = std::make_shared<polysync::formatter::plain>();
    else
        polysync::format = std::make_shared< polysync::formatter::fancy >();

    // Check that the environment is set up so the plugins do not have to.
    char* libdir = std::getenv( "POLYSYNC_TRANSCODER_LIB" );
    if ( libdir == nullptr ) {
        throw polysync::error(
                "POLYSYNC_TRANSCODER_LIB unset; cannot find TOML or plugins");
    }

    // Find all the runtime resources and load them
    ps::toml::load( toml_options );
    ps::filter::load( filter_options );
    ps::encode::load( encode_options );

}

po::parsed_options cmdline_parser::configure_input_output( int ac, char* av[] ) {

    // Stage 2: determine the filters and encoder name
    po::positional_options_description posdesc;
    posdesc.add( "input", 1 ).add( "encoder", 1 ).add( "encoder_args", -1 );

    po::options_description positional_options;
    positional_options.add_options()
        ( "input", po::value< std::vector<fs::path> >(), "Input file" )
        ( "encoder", po::value<std::string>()->default_value( "list" ),
          "Output encoder" )
        ( "encoder_args", po::value< std::vector<std::string> >(),
          "Encoder arguments" )
        ;

    po::options_description options;
    options.add( bootstrap_options )
           .add( toml_options )
           .add( filter_options )
           .add( positional_options )
        ;

    po::parsed_options parse_results = po::command_line_parser( ac, av )
        .options(options)
        .positional(posdesc)
        .allow_unregistered() // Pass through encoder args for
        .run();
    po::store( parse_results, cmdline_args );
    po::notify( cmdline_args );

    if ( !cmdline_args.count("input") ) {
        throw ps::error( "no input file" ) << ps::status::bad_input;
    }

    std::string encoder = cmdline_args["encoder"].as<std::string>();
    if ( !ps::encode::map.count( encoder ) ) {
        std::cerr << "error: unknown encoder \"" << encoder << "\"" << std::endl;
        exit( ps::status::no_plugin );
    }

    return parse_results;
}

void cmdline_parser::configure_encoder( po::parsed_options& parse_results ) {

    // Stage 3:
    // Alt this point, all the general options are parsed, as are the positional
    // arguments for input and encoder name. Build a final stage 3 parse that is
    // informed by the specific encoder requested.

    std::string encoder = cmdline_args["encoder"].as<std::string>();
    po::options_description options = ps::encode::map.at( encoder )->options();
    std::vector<std::string> encoder_args =
        po::collect_unrecognized( parse_results.options, po::include_positional );

    // Parse again, with now with encoder options
    po::store( po::command_line_parser( encoder_args )
            .options( options )
            .run(), cmdline_args );
    po::notify( cmdline_args );
}


int catch_main( int ac, char* av[] ) {

    logger log { "transcode" };

    cmdline_parser parse;

    parse.bootstrap( ac, av );
    parse.print_usage();
    po::parsed_options parse_results = parse.configure_input_output( ac, av );
    parse.configure_encoder( parse_results );

    std::vector<ps::filter::type> filters;
    // The filter arguments are parsed.  Build a list of active filters.
    for ( auto pair: ps::filter::map ) {
        ps::filter::type pred = pair.second->predicate( parse.cmdline_args );
        if ( pred ) {
            filters.push_back(pred);
        }
    }

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
            // Don't bother registering detectors for types if do not have the
            // descriptor.
            if ( ps::descriptor::catalog.count(t.name) ) {
                 ps::detector::catalog.push_back(ps::detector::type { "msg_header",
                    { { "type", t.type } },
                    t.name } );
                 }
                 plog::type_support_map.emplace( t.type, t.name );
            } );

    std::string encoder = parse.cmdline_args["encoder"].as<std::string>();
    ps::encode::map.at( encoder )->connect( parse.cmdline_args, visit );

    // The observers are finally all set up.  Here, we finally do the computation!
    // Double iterate over files from the command line, and records in each file.
    for ( fs::path path: parse.cmdline_args["input"].as< std::vector<fs::path> >() )
    {
        try {
            std::ifstream st( path.c_str(), std::ifstream::binary );
            if ( !st ) {
                throw polysync::error( "cannot open file" );
            }

            // Construct the next reader in the file list
            plog::decoder decoder( st );

            visit.open( decoder );

            plog::log_header head;

            decoder.decode( head );
            visit.log_header( head );

            for ( const plog::type_support& type: head.type_supports ) {
                visit.type_support( type );
            }

            for ( const plog::log_record& rec: decoder ) {
                if ( std::all_of( filters.begin(), filters.end(),
                      [&rec]( const ps::filter::type& pred ) { return pred(rec); } ) ) {
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
        if ( const ps::status* stat =
                boost::get_error_info<ps::exception::status>(e) ) {
            exit( *stat );
        }
    } catch ( const po::error& e ) {
        std::cerr << polysync::format->error( "Transcoder abort: " )
                  << e.what() << std::endl;
        return polysync::status::bad_argument;
    }
}

