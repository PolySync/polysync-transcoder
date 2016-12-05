#include <polysync/plog/core.hpp>
#include <polysync/plog/decoder.hpp>
#include <polysync/detector.hpp>
#include <polysync/plugin.hpp>
#include <polysync/logging.hpp>
#include <polysync/exception.hpp>
#include <polysync/console.hpp>
#include <polysync/print_hana.hpp>

// Use boost::dll and boost::filesystem to manage plugins.  Among other
// benefits, it eases a Windows port.
#include <boost/dll/import.hpp>
#include <boost/dll/shared_library.hpp>
#include <boost/dll/runtime_symbol_info.hpp> // for program_location()
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>

#include <iostream>
#include <algorithm>
#include <regex>

namespace po = boost::program_options;
namespace fs = boost::filesystem;
namespace dll = boost::dll;
namespace plog = polysync::plog;
namespace ps = polysync;

using ps::logging::severity;
using ps::logging::logger;

namespace std {

// boost::program_options likes to print the arguments, so here we teach it how
// to print out a vector of fs::path.
std::ostream& operator<<(std::ostream& os, const std::vector<fs::path>& paths) {
    std::for_each(paths.begin(), paths.end(), [&os](const fs::path& p) { os << p << " "; });
    return os;
}

}

std::map<std::string, boost::shared_ptr<ps::encode::plugin>> plugin_map;

int catch_main(int ac, char* av[]) {

    logger log("transcode");

    using polysync::format;

    // By default, use a fancy formatter.  This can change below.
    format = std::make_shared<polysync::formatter::fancy>();

    // Define a rather sophisticated command line parse that supports
    // subcommands (rather like git) where each subcommand is a supported
    // output driver (like CSV, HDF5, or SciDB).  Originally described by
    // http://stackoverflow.com/questions/15541498/how-to-implement-subcommands-using-boost-program-options
    
    // compute reasonable guess about plugin and share locations.
    char exebuf[1024];
    size_t exelen = readlink("/proc/self/exe", exebuf, 1024); // This is my own path, compliments of /proc.
    fs::path exe(std::string(exebuf, exelen));

    po::options_description general_opt("General Options");
    general_opt.add_options()
        ("help,h", "print this help message")
        ("verbose,v", po::value<std::string>()->default_value("info"), "debug level")
        ("plain,p", "remove color from formatting")
        ("plugdir,P", po::value<std::vector<fs::path>>()
                ->default_value(std::vector<fs::path>{ 
                    "./plugin", exe.parent_path() / "../plugin" })
                ->composing(),
                "plugin path last")
        ("descdir,D", po::value<std::vector<fs::path>>()
                ->default_value(std::vector<fs::path>{
                    "../share", exe.parent_path() / "../../share" })
                ->composing()
                ->multitoken(),
                "TOFL type description path list")
        ;

    // These will move to a filters plugin.
    po::options_description filter_opt("Filter Options");
    filter_opt.add_options()
        ("slice", po::value<std::string>(), "<begin:stride:end> Numpy style record slice syntax")
        ;

    po::options_description positional_opt;
    positional_opt.add_options()
        ("path", po::value<std::vector<fs::path>>(), "plog input files")
        ("output", po::value<std::string>()->default_value("list"), "Output formatter")
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

    // Set the debug and console format options right away
    ps::logging::set_level(vm["verbose"].as<std::string>());

    if (vm.count("plain"))
        format = std::make_shared<polysync::formatter::plain>();

    // We have some hard linked plugins.  This makes important ones and that
    // have no extra dependencies always available, even if the plugin path is
    // misconfigured.  It also makes the query options appear first in the help
    // screen, also useful.
    dll::shared_library self(dll::program_location());
    for (std::string plugname: { "list", "dump" } ) {
        auto plugin_factory = self.get_alias<boost::shared_ptr<ps::encode::plugin>()>(plugname + "_plugin");
        auto plugin = plugin_factory();
        po::options_description opt = plugin->options();
        helpline.add(opt);
        cmdline.add(opt);
        plugin_map.emplace(plugname, plugin);
    }

    // Iterate the plugin path, and for each path load every valid entry in
    // that path.  Add options to parser.
    for (fs::path plugdir: vm["plugdir"].as<std::vector<fs::path>>()) {

        if (!fs::exists(plugdir)) {
            BOOST_LOG_SEV(log, severity::debug1) << "skipping non-existing plugin path " << plugdir;
            continue;
        }

        BOOST_LOG_SEV(log, severity::debug1) << "searching " << plugdir << " for plugins";
        static std::regex is_plugin(R"(.*transcode\.(.+)\.so)");
        for (fs::directory_entry& lib: fs::directory_iterator(plugdir)) {
            std::cmatch match;
            std::regex_match(lib.path().string().c_str(), match, is_plugin);
            if (match.size()) {
                std::string plugname = match[1];

                if (plugin_map.count(plugname)) {
                    BOOST_LOG_SEV(log, severity::debug2) << "encoder \"" << plugname << "\"already loaded";
                    continue;
                }

                try {
                    boost::shared_ptr<ps::encode::plugin> plugin = 
                        dll::import<ps::encode::plugin>(lib.path(), "encoder");
                    BOOST_LOG_SEV(log, severity::debug1) << "loaded encoder from " << lib.path();
                    po::options_description opt = plugin->options();
                    helpline.add(opt);
                    cmdline.add(opt);
                    plugin_map.emplace(plugname, plugin);
                } catch (std::runtime_error&) {
                    BOOST_LOG_SEV(log, severity::debug2) 
                        << lib.path() << " provides no encoder; symbols not found";
                }
            } else {
                BOOST_LOG_SEV(log, severity::debug2) 
                    << lib.path() << " provides no encoder; filename mismatch";
            }
        }
    }

    if (vm.count("help")) {
        std::cout << format->header("PolySync Transcoder") << std::endl << std::endl;
        std::cout << "Usage:" << std::endl;
        std::cout << "\ttranscode [general-options] <input-file> <output-plugin> [output-options]" 
                  << std::endl;
        std::cout << helpline << std::endl << std::endl;
        return ps::status::ok;
    }


    for (fs::path descdir: vm["descdir"].as<std::vector<fs::path>>()) {

        if (!fs::exists(descdir)) {
            BOOST_LOG_SEV(log, severity::debug1) << "skipping description path " << descdir 
                << " because it does not exist";
            continue;
        }

        BOOST_LOG_SEV(log, severity::debug1) << "searching " << descdir << " for type descriptions";
        static std::regex is_description(R"((.+)\.toml)");
        for (fs::directory_entry& tofl: fs::directory_iterator(descdir)) {
            std::cmatch match;
            std::regex_match(tofl.path().string().c_str(), match, is_description);
            if (match.size()) {
                BOOST_LOG_SEV(log, severity::debug1) << "loading descriptions from " << tofl;
                try {
                    std::shared_ptr<cpptoml::table> descfile = cpptoml::parse_file(tofl.path().string());

                    // Parse the file in two passes, so the detectors have
                    // access to the descriptor's types
                    for (const auto& type: *descfile) {
                        if (type.second->is_table())
                            ps::descriptor::load(
                                    type.first, type.second->as_table(), ps::descriptor::catalog);
                        else if (type.second->is_value()) {
                            auto val = type.second->as<std::string>();
                            if (!ps::descriptor::namemap.count(val->get()))
                                throw ps::error("unknown type alias") 
                                    << ps::exception::type(type.first);
                            std::type_index idx = ps::descriptor::namemap.at(val->get());
                            ps::descriptor::namemap.emplace(type.first, idx);
                            BOOST_LOG_SEV(log, severity::debug2) 
                                << "loaded type alias " << type.first << " = " << val->get();
                        }
                        else
                            BOOST_LOG_SEV(log, severity::warn) 
                                << "unused description: " << type.first;
                    }
                    for (const auto& type: *descfile)
                        if (type.second->is_table())
                            ps::detector::load(
                                    type.first, type.second->as_table(), ps::detector::catalog);
                } catch (ps::error& e) {
                    e << ps::status::description_error;
                    e << ps::exception::path(tofl.path().string());
                    throw;
                } catch (cpptoml::parse_exception& e) {
                    throw polysync::error(e.what()) << ps::status::description_error
                        << ps::exception::path(tofl.path().string());
                }
            }
        }
    }

    if (!vm.count("path")) 
        throw ps::error("no input files") << ps::status::bad_input;

    std::string output = vm["output"].as<std::string>();
    if (!plugin_map.count(output)) {
        std::cerr << "error: unknown output \"" << output << "\"" << std::endl;
        exit(ps::status::no_plugin);
    }

    std::vector<std::string> opts = po::collect_unrecognized(parse.options, po::include_positional);
    if (!opts.empty()) {
        opts.erase(opts.begin());

        po::options_description output_opt = plugin_map.at(output)->options();

        // Parse again, with now with output options
        po::store(po::command_line_parser(opts).options(output_opt).run(), vm);
        po::notify(vm);
    }

    ps::descriptor::catalog.emplace("log_record", ps::descriptor::describe<plog::log_record>::type());
    ps::descriptor::catalog.emplace("msg_header", ps::descriptor::describe<plog::msg_header>::type());

    // Set observer patterns, with the subject being the iterated plog readers
    // and the iterated records from each.  The observers being callbacks
    // requested in the command line arguments
    ps::encode::visitor visit;

    // PLogs key each message type by a number, but the number is generally
    // different in every plog.  Constant names are provided in the header in
    // the type_support field; stash them so we can map the (random) numbers to
    // (useful) static strings.  Most if not every plugin needs these, so just
    // add it globally here.
    visit.type_support.connect([](plog::type_support t) { 
            // Don't bother registering detectors for types if do not have the descriptor.
            if (ps::descriptor::catalog.count(t.name))
                ps::detector::catalog.push_back(ps::detector::type { "msg_header", 
                    { { "type", t.type } }, 
                    t.name });
            plog::type_support_map.emplace(t.type, t.name); 
            });

    plugin_map.at(output)->connect(vm, visit);

    // These rudimentary filters will move to a filter plugin
    std::function<bool (const plog::log_record&)> filter = [](const plog::log_record&) { return true; };
    if (vm.count("slice")) {
        // Emulate the the excellent Python Numpy slicing syntax
        static std::regex slice_re(R"((\d+)?(:)?(\d+)?(:)?(\d+)?)");
        std::smatch slice;
        if (std::regex_match(vm["slice"].as<std::string>(), slice, slice_re)) {
            if (slice[4].matched && !slice[5].matched)
                throw polysync::error("bad slice format") << polysync::status::bad_argument;

            size_t begin = slice[1].matched ? std::stol(slice[1]) : 0;
            size_t end = slice[2].matched ? -1 : begin + 1;
            size_t stride = slice[4].matched ? std::stol(slice[3]) : 1;
            end = slice[5].matched ? std::stol(slice[5]) : end;
            end = (slice[3].matched && !slice[4].matched) ? std::stol(slice[3]) : end;

            BOOST_LOG_SEV(log, severity::debug2) << "slice " << begin << ":" << stride << ":" << end;
            filter = [filter, begin, end](const plog::log_record& rec) {
                return filter(rec) && (rec.index >= begin) && (rec.index < end);
            };
        }
        else
            throw polysync::error("bad slice format") << polysync::status::bad_argument;
        
    }
    
    // The observers are finally all set up.  Here, we finally do the computation!
    // Double iterate over files from the command line, and records in each file.
    for (fs::path path: vm["path"].as<std::vector<fs::path>>()) 
    {
        try {
            std::ifstream st(path.c_str(), std::ifstream::binary);
            if (!st)
                throw polysync::error("cannot open file");

            // Construct the next reader in the file list
            plog::decoder decoder(st);

            visit.open(decoder);

            plog::log_header head;

            decoder.decode(head);
            visit.log_header(head);
            for (const plog::type_support& type: head.type_supports)
                visit.type_support(type);
            for (const plog::log_record& rec: decoder) {
                if (filter(rec)) {
                    BOOST_LOG_SEV(log, severity::verbose) << rec;
                    polysync::node top("log_record", decoder.deep(rec));
                    visit.record(top);
                }
            }
            visit.cleanup(decoder);
        } catch (polysync::error& e) {
            e << ps::exception::path(path.c_str());
            e << polysync::status::bad_input;
            throw;
        }
    }

    // We seemed to have survive the run!
    return ps::status::ok;
}

int main(int ac, char* av[]) {
    try {
        return catch_main(ac, av);
    } catch (const ps::error& e) {
        // Print any context provided by the exception.
        std::cerr << polysync::format->error("Transcoder abort: ") << e;
        if (const ps::status* stat = boost::get_error_info<ps::exception::status>(e)) 
            exit(*stat);
    } catch (const po::error& e) {
        std::cerr << polysync::format->error("Transcoder abort: ") << e.what() << std::endl;
        return polysync::status::bad_argument;
    }
}

