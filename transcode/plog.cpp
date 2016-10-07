#include <polysync/transcode/plugin.hpp>
#include <polysync/transcode/writer.hpp>
#include <boost/make_shared.hpp>
#include <boost/dll/alias.hpp>
#include <boost/filesystem.hpp>

#include <polysync/transcode/io.hpp>

namespace polysync { namespace transcode { namespace plog {

namespace po = boost::program_options;
namespace fs = boost::filesystem;
namespace hana = boost::hana;
using namespace polysync::plog;

static std::shared_ptr<polysync::plog::writer> writer;

struct plugin : public transcode::plugin { 

    po::options_description options() const {
        po::options_description opt("PLog Options");
        opt.add_options()
            ("name,n", po::value<fs::path>(), "filename")
            ;
        return opt;
    }

    void observe(const po::variables_map& vm, callback& call) const {
        using reader = polysync::plog::reader;
        writer.reset(new polysync::plog::writer(vm["name"].as<fs::path>().string()));

        call.reader.connect([](const reader& r) { writer->write(r.get_header()); });
        call.type_support.connect([](const plog::type_support& t) { });
        call.record.connect([](const log_record& rec) { 
                // std::cout << rec << std::endl;
                writer->write(rec); 
                });
    }
};

boost::shared_ptr<transcode::plugin> create_plugin() {
    return boost::make_shared<struct plugin>();
}

}}} // namespace transcode::plog

BOOST_DLL_ALIAS(polysync::transcode::plog::create_plugin, plog_plugin)

