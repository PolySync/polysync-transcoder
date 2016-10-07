#include <polysync/transcode/plugin.hpp>
#include <polysync/transcode/io.hpp>
#include <boost/make_shared.hpp>
#include <boost/dll/alias.hpp>

namespace polysync { namespace transcode { namespace query {

namespace po = boost::program_options;

struct plugin : transcode::plugin { 

    po::options_description options() const {
        po::options_description opt("Query Options");
        opt.add_options()
            ("name", "print filename to stdout")
            ("version", "dump type support to stdout")
            ("types", "dump type support to stdout")
            ("modules", "dump modules to stdout")
            ("count", "print number of records to stdout")
            ("headers", "dump header summaries to stdout")
            ("dump", "dump entire record to stdout")
            ;

        return opt;
    }

    void observe(const po::variables_map& vm, callback& call) const {

        std::map<plog::ps_msg_type, std::string> msg_type_map;
        call.type_support.connect([&msg_type_map](const plog::type_support& t) {
                std::cout << t.type << ": " << t.name << std::endl;
                msg_type_map.emplace(t.type, t.name);
                });

        // Dump summary information of each file
        if (vm.count("name"))
            call.reader.connect([](auto&& log) { std::cout << log.get_filename() << std::endl; });

        if (vm.count("version"))
            call.reader.connect([](auto&& log) { std::cout 
                    << "PLog Version " 
                    << std::to_string(log.get_header().version_major) << "." 
                    << std::to_string(log.get_header().version_minor) << "." 
                    << std::to_string(log.get_header().version_subminor) << ", "
                    << "built " << log.get_header().build_date << std::endl; });

        if (vm.count("modules"))
            call.reader.connect([](auto&& log) { std::cout << log.get_header().modules << std::endl; });

        if (vm.count("types"))
            call.reader.connect([](auto&& log) { std::cout << log.get_header().type_supports << std::endl; });

        // Dump one line headers from each record to stdout.
        if (vm.count("headers"))
            call.record.connect([&msg_type_map](auto record) { 
                    std::cout << record.index << ": " << record.size << " bytes, type " 
                    << msg_type_map.at(record.msg_header.type)
                    << std::endl; 
                    });

        // Dump entire records to stdout
        if (vm.count("dump"))
            call.record.connect([](auto record) { std::cout << record << std::endl; });

        // Count records
        size_t count = 0;
        if (vm.count("count")) {
            call.record.connect([&count](auto&&) { count += 1; });
            call.cleanup.connect([&count](auto&&) { 
                    std::cout << count << " records" << std::endl; 
                    count = 0;
                    });
        }

    }


};


boost::shared_ptr<transcode::plugin> create_plugin() {
    return boost::make_shared<struct plugin>();
}

}}} // namespace polysync::transcode::query

BOOST_DLL_ALIAS(polysync::transcode::query::create_plugin, query_plugin)


