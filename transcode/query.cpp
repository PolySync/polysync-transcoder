#include <polysync/transcode/plugin.hpp>
#include <polysync/transcode/dynamic_reader.hpp>
#include <polysync/transcode/io.hpp>
#include <boost/make_shared.hpp>
#include <boost/dll/alias.hpp>

namespace polysync { namespace transcode { namespace query {

namespace po = boost::program_options;
namespace endian = boost::endian;

struct plugin : transcode::plugin { 

    po::options_description options() const {
        po::options_description opt("Query Options");
        opt.add_options()
            ("version", "dump type support to stdout")
            ("types", "dump type support to stdout")
            ("modules", "dump modules to stdout")
            ("count", "print number of records to stdout")
            ("headers", "dump header summaries to stdout")
            ("dump", "dump entire record to stdout")
            ;

        return opt;
    }

    void connect(const po::variables_map& vm, callback& call) const {

        // Dump summary information of each file
        // if (vm.count("version"))
        //     call.reader.connect([](auto&& log) { std::cout 
        //             << "PLog Version " 
        //             << std::to_string(log.get_header().version_major) << "." 
        //             << std::to_string(log.get_header().version_minor) << "." 
        //             << std::to_string(log.get_header().version_subminor) << ", "
        //             << "built " << log.get_header().build_date << std::endl; });

        // if (vm.count("modules"))
        //     call.reader.connect([](auto&& log) { std::cout << log.get_header().modules << std::endl; });

        // if (vm.count("types"))
        //     call.reader.connect([](auto&& log) { std::cout << log.get_header().type_supports << std::endl; });

        // Dump one line headers from each record to stdout.
        if (vm.count("headers"))
            call.record.connect([](auto record) { 
                    std::cout << record.index << ": " << record.size << " bytes, type " 
                    // << msg_type.at(record.msg_header.type)
                    << std::endl; 
                    });

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


