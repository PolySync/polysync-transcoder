#include <polysync/transcode/core.hpp>
#include <polysync/transcode/reader.hpp>
#include <polysync/transcode/plugin.hpp>

// Use boost::dll and boost::filesystem to manage plugins.  Among other
// benefits, it eases a Windows port.
#include <boost/dll/import.hpp>
#include <boost/dll/runtime_symbol_info.hpp> // for program_location()
#include <boost/filesystem.hpp>
#include <boost/hana/map.hpp>
#include <boost/hana/pair.hpp>
#include <boost/hana/type.hpp>

#include <boost/program_options.hpp>
#include <iostream>
#include <algorithm>
#include <regex>

namespace po = boost::program_options;
namespace fs = boost::filesystem;
namespace dll = boost::dll;
namespace hana = boost::hana;
namespace plog = polysync::plog;

int main(int ac, char* av[]) {

    // Define a rather sophisticated command line parse that supports
    // subcommands (rather like git) where each subcommand is a supported
    // output driver (like CSV, HDF5, or SciDB).  Originally described by
    // http://stackoverflow.com/questions/15541498/how-to-implement-subcommands-using-boost-program-options
    
    po::options_description general_opt("General Options");
    general_opt.add_options()
        ("help,h", "print this help message")
        ("plug,p", po::value<std::vector<fs::path>>()
         ->default_value(std::vector<fs::path>(), TRANSCODE_PLUGIN_PATH)->composing(),
         "configure plugin path")
        ("verbose,v", "print transcode diagnostic info")
        ;

    po::options_description filter_opt("Filter Options");
    filter_opt.add_options()
        // not sure yet how to implement these
        ("first", po::value<size_t>(), "first record index to process (not yet implemented)")
        ("last", po::value<size_t>(), "last record index to process (not yet implemented)")
        ;

    po::options_description positional_opt;
    positional_opt.add_options()
        ("path", po::value<std::vector<fs::path>>(), "plog input files")
        ("output", po::value<std::string>(), "Output formatter")
        ("subargs", po::value<std::vector<std::string>>(), "Arguments for output formatter")
        ;

    po::positional_options_description posdesc;
    posdesc.add("path", 1).add("output", 1).add("subargs", -1);

    po::options_description cmdline, helpline;  // These are the same except for positional_opt.
    cmdline.add(general_opt).add(filter_opt).add(positional_opt);
    helpline.add(general_opt).add(filter_opt);

    // Do the first of two parses.  This first one is required to discover the
    // plugin path and the subcommand name.
    po::variables_map vm;
    po::parsed_options parse = po::command_line_parser(ac, av)
        .options(cmdline)
        .positional(posdesc)
        .allow_unregistered()
        .run();
    po::store(parse, vm);
    po::notify(vm);

    std::map<std::string, boost::shared_ptr<polysync::transcode::plugin>> plugin_map;
    // We have two hard linked plugins: query and csv.  This makes important
    // ones (like query) and that have no extra dependencies always available,
    // even if the plugin path is misconfigured.  It also makes the query
    // options appear first in the help screen, also useful.
    dll::shared_library self(dll::program_location());
    for (std::string plugname: { "query", "plog", "csv" } ) {
        auto plugin_factory = self.get_alias<boost::shared_ptr<polysync::transcode::plugin>()>(plugname + "_plugin");
        auto plugin = plugin_factory();
        po::options_description opt = plugin->options();
        helpline.add(opt);
        cmdline.add(opt);
        plugin_map.emplace(plugname, plugin);
    }

    // Iterate the plugin path, and for each path load every valid entry in
    // that path.  Add options to parser.
    for (fs::path plugdir: vm["plug"].as<std::vector<fs::path>>()) {
        if (vm.count("verbose"))
            std::cerr << "searching " << plugdir << " for plugins" << std::endl;
        static std::regex is_plugin(R"(.*transcode\.(.+)\.so)");
        for (fs::directory_entry& lib: fs::directory_iterator(plugdir)) {
            std::cmatch match;
            std::regex_match(lib.path().string().c_str(), match, is_plugin);
            if (match.size()) {
                std::string plugname = match[1];
                if (vm.count("verbose"))
                    std::cerr << "loading plugin " << plugname << " from " << lib.path() << std::endl;
                boost::shared_ptr<polysync::transcode::plugin> plugin = 
                    dll::import<polysync::transcode::plugin>(lib.path(), "plugin");
                po::options_description opt = plugin->options();
                cmdline.add(opt);
                helpline.add(opt);
                plugin_map.emplace(plugname, plugin);
            }
        }
    }

    if (vm.count("help")) {
        std::cout << "PolySync Transcoder" << std::endl << std::endl;
        std::cout << "Usage:" << std::endl;
        std::cout << "\ttranscode [options] <plog> <output> [options]" << std::endl;
        std::cout << helpline << std::endl << std::endl;
        return true;
    }

    std::string output;

    if (vm.count("output"))
        output = vm["output"].as<std::string>();
    else
        std::cerr << "warning: no output plugin requested!" << std::endl;

    if (!plugin_map.count(output)) {
        std::cerr << "error: no plugin " << output << std::endl;
        exit(-1);
    }

    if (vm.count("verbose"))
        std::cerr << "configuring plugin " << output << std::endl;

    po::options_description output_opt = plugin_map.at(output)->options();

    std::vector<std::string> opts = po::collect_unrecognized(parse.options, po::include_positional);
    if (!opts.empty()) {
        opts.erase(opts.begin());

        // Parse again, with now with output options
        po::store(po::command_line_parser(opts).options(output_opt).run(), vm);
    }

    // Set observer patterns, with the subject being the iterated plog readers
    // and the iterated records from each.  The observers being callbacks
    // requested in the command line arguments
    polysync::transcode::callback call;

    if (!vm.count("path")) {
        std::cerr << "error: no plog paths supplied!" << std::endl;
        return -1;
    }

    plugin_map.at(output)->observe(vm, call);

    std::function<bool (plog::iterator)> filter = [](plog::iterator it) { return true; };
    if (vm.count("first")) {
        size_t first = vm["first"].as<size_t>(); 
        filter = [filter, first](plog::iterator it) { return filter(it) && ((*it).index >= first); }; 
    }
    if (vm.count("last")) {
        size_t last = vm["last"].as<size_t>(); 
        filter = [filter, last](plog::iterator it) { return filter(it) && ((*it).index < last); }; 
    }

    // The observers are finally all set up.  Here, we finally do the computation!
    // Double iterate over files from the command line, and records in each file.
    for (fs::path path: vm["path"].as<std::vector<fs::path>>()) 
    {
        // Construct the next reader in the file list
        plog::reader reader(path.c_str());

        call.reader(reader);

        const plog::log_header& head = reader.get_header();
        std::for_each(head.type_supports.begin(), head.type_supports.end(), std::ref(call.type_support));

        if (!call.record.empty()) 
            std::for_each(reader.begin(filter), reader.end(), std::ref(call.record));

        call.cleanup(reader);
    }
}
