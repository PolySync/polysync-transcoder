#include <iostream>
#include <algorithm>
#include <regex>
#include <cstdlib>

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>

#include <polysync/plog/core.hpp>
#include <polysync/plog/core.hpp>
#include <polysync/plog/decoder.hpp>
#include <polysync/plog/magic.hpp>
#include <polysync/plugin.hpp>
#include <polysync/toml.hpp>
#include <polysync/descriptor.hpp>
#include <polysync/detector.hpp>
#include <polysync/logging.hpp>
#include <polysync/exception.hpp>
#include <polysync/console.hpp>
#include <polysync/print_hana.hpp>
#include <polysync/size.hpp>

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
    std::for_each( paths.begin(), paths.end(), [&os](const fs::path& p) { os << p << " "; } );
    return os;
}

} // namespace std

int catch_main( int ac, char* av[] ) {

    logger log( "transcode" );

    using polysync::format;

    // By default, use a fancy formatter.  This can change below.
    format = std::make_shared< polysync::formatter::fancy >();

    // The command line parse is complicated because the plugins are allowed to
    // add options.  There are three stages:
    // 1) Configure the console spew because this will affect the second stage.
    // 2) Parse the positional arguments (input and encoder) to identify the
    //    encoder's name.  Filters are also parsed in this stage.  Filter
    //    options cannot be duplicated; take care in the filter plugins that
    //    the option names are not reserved by some other plugin.
    // 3) The final parse enables only the options from the chosen encoder.  This
    //    allows duplicate options between different encoder plugins, which will
    //    be common (like output file name).
    //
    // The encoders are handled similarly to subcommands (like git does).  This
    // pattern was originally described for boost::program_options by
    // http://stackoverflow.com/questions/15541498/how-to-implement-subcommands-using-boost-program-options

    // All the stages accumulate results into one container of arguments,
    // called cmdline_args.
    po::variables_map cmdline_args;

    // Stage 1:  Console options
    po::options_description console_opts( "General Options" );
    console_opts.add_options()
        ( "help,h", "print this help message" )
        ( "loglevel,d", po::value<std::string>()
            ->default_value("info"), "log level" )
        ( "plain,p", "remove color from console formatting" )
        ( "plugdir,P", po::value< std::vector<fs::path> >()
            ->default_value( std::vector<fs::path>() )
            ->composing(),
            "additional plugin directories (augments POLYSYNC_TRANSCODE_LIB")
        ;

    po::parsed_options stage1_parse = po::command_line_parser( ac, av )
        .options(console_opts)
        .allow_unregistered() // Pass through everything except console_opts
        .run();
    po::store( stage1_parse, cmdline_args );
    po::notify( cmdline_args );

    // Set the debug and console format options right away so log messages in
    // the subsequent plugin loaders are processed correctly.
    ps::logging::set_level( cmdline_args["loglevel"].as<std::string>() );

    if (cmdline_args.count("plain"))
        format = std::make_shared<polysync::formatter::plain>();

    // Configure the runtime environment to find the plugins
    std::vector<fs::path> plugpath =
        cmdline_args["plugdir"].as< std::vector<fs::path> >();
    char* libenv = std::getenv( "POLYSYNC_TRANSCODER_LIB" );
    if ( libenv != nullptr )
        boost::split( plugpath, libenv, boost::is_any_of(":;") );

#ifdef INSTALL_PREFIX
    plugpath.push_back( INSTALL_PREFIX );
#endif

    if ( plugpath.empty() )
        BOOST_LOG_SEV(log, severity::warn) << "plugin path unset;"
            << " use POLYSYNC_TRANSCODE_LIB or --plugdir to find them";

    // Find all the runtime resources and load them
    po::options_description toml_opts = ps::toml::load( plugpath );
    po::options_description filter_opts = ps::filter::load( plugpath );
    po::options_description encode_opts = ps::encode::load( plugpath );

    // Build the help spew.  toml_opts is omitted because, for now, it is actually empty.
    po::options_description helpline;
    helpline.add( console_opts ).add( filter_opts ).add( encode_opts );

    if ( cmdline_args.count("help") ) {
        std::cout << format->header( "PolySync Transcoder" ) << std::endl << std::endl;
        std::cout << "Usage:" << std::endl;
        std::cout << "\ttranscode [options] <input-file> <encoder> [encoder-options]" << std::endl;
        std::cout << helpline << std::endl << std::endl;
        return ps::status::ok;
    }

    // Stage 2: determine the filters and encoder name
    po::options_description positional_opts;
    positional_opts.add_options()
        ( "input", po::value<std::vector<fs::path>>(), "Input file" )
        ( "encoder", po::value<std::string>()->default_value("list"), "Output encoder" )
        ( "subargs", po::value<std::vector<std::string>>(), "Encoder arguments" )
        ;

    po::positional_options_description posdesc;
    posdesc.add( "input", 1 ).add( "encoder", 1 ).add( "subargs", -1 );

    po::options_description stage2_cmdline;
    stage2_cmdline.add( console_opts ).add( filter_opts ).add( positional_opts );

    po::parsed_options stage2_parse = po::command_line_parser( ac, av )
        .options(stage2_cmdline)
        .positional(posdesc)
        .allow_unregistered() // Pass through all the encoder arguments, awaiting stage 3 pass
        .run();
    po::store( stage2_parse, cmdline_args );
    po::notify( cmdline_args );

    if ( !cmdline_args.count("input") )
        throw ps::error( "no input file" ) << ps::status::bad_input;

    std::string encoder = cmdline_args["encoder"].as<std::string>();
    if ( !ps::encode::map.count( encoder ) ) {
        std::cerr << "error: unknown encoder \"" << encoder << "\"" << std::endl;
        exit( ps::status::no_plugin );
    }

    // The filter arguments are parsed.  Build a list of active filters.
    std::vector<ps::filter::type> filters;
    for ( auto pair: ps::filter::map ) {
        ps::filter::type pred = pair.second->predicate( cmdline_args );
        if ( pred )
            filters.push_back(pred);
    }

    // Stage 3:
    // At this point, all the general options are parsed, as are the positional
    // arguments for input and encoder name. Build a final stage 3 parse that is
    // informed by the specific encoder requested.

    po::options_description encoder_opts = ps::encode::map.at( encoder )->options();
    po::options_description stage3_cmdline;
    stage3_cmdline.add( encoder_opts );

    std::vector<std::string> encoder_args =
        po::collect_unrecognized( stage2_parse.options, po::include_positional );

    ps::plog::load();

    // Parse again, with now with encoder options
    po::store( po::command_line_parser( encoder_args ).options( stage3_cmdline ).run(), cmdline_args );
    po::notify(cmdline_args);

    // Set observer patterns, with the subject being the iterated plog readers
    // and the iterated records from each.  The observers being callbacks
    // requested in the command line arguments
    ps::encode::visitor visit;

    // PLogs key each message type by a number, but the number is generally
    // different in every plog.  Constant names are provided in the header in
    // the type_support field; stash them so we can map the (random) numbers to
    // (useful) static strings.  Most if not every plugin needs these, so just
    // add it globally here.
    visit.type_support.connect( []( plog::ps_type_support t ) {
            // Don't bother registering detectors for types if do not have the descriptor.
            if ( ps::descriptor::catalog.count(t.name) )
                 ps::detector::catalog.push_back(ps::detector::Type { "ps_msg_header",
                    { { "type", t.type } },
                    t.name } );
            // plog::type_support_map.emplace( t.type, t.name );
            } );

    ps::encode::map.at( encoder )->connect( cmdline_args, visit );

    // The observers are finally all set up.  Here, we finally do the computation!
    // Double iterate over files from the command line, and records in each file.
    for ( fs::path path: cmdline_args["input"].as<std::vector<fs::path>>() )
    {
        try {
            std::ifstream st( path.c_str(), std::ifstream::binary );
            if ( !st )
                throw polysync::error( "cannot open file" );

            if ( !plog::checkMagic( st ) )
            {
                throw polysync::error( "input file not a plog" );
            }

            // Construct the next reader in the file list
            plog::decoder decoder( st );

            visit.open( decoder );

            plog::ps_log_header head;

            decoder.decode( head );
            visit.log_header( head );
            for ( const plog::ps_type_support& type: head.type_supports )
                visit.type_support( type );
            for ( const plog::ps_log_record& rec: decoder ) {
                if ( std::all_of(filters.begin(), filters.end(),
                            [&rec]( const ps::filter::type& pred ) { return pred(rec); }) ) {
                    BOOST_LOG_SEV( log, severity::verbose ) << rec;
                    polysync::node top( "ps_log_record", decoder.deep(rec) );
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

