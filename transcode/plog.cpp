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

static std::ofstream out;
static std::shared_ptr<polysync::plog::writer> writer;

struct plugin : public transcode::plugin { 

    po::options_description options() const {
        po::options_description opt("PLog Options");
        opt.add_options()
            ("name,n", po::value<fs::path>(), "filename")
            ;
        return opt;
    }

    void connect(const po::variables_map& vm, callback& call) const {

        using reader = polysync::plog::reader;
        std::string path = vm["name"].as<fs::path>().string();

        call.reader.connect([path](reader& r) { 
            out.open(path, std::ios_base::out | std::ios_base::binary);
            writer.reset(new polysync::plog::writer(out));

            plog::log_header head;
            r.read(head);
            writer->write(head); 
            });

        call.type_support.connect([](const plog::type_support& t) { });
        call.record.connect([](const log_record& rec) { writer->write(rec); });
    }
};

boost::shared_ptr<transcode::plugin> create_plugin() {
    return boost::make_shared<struct plugin>();
}

}}} // namespace transcode::plog

BOOST_DLL_ALIAS(polysync::transcode::plog::create_plugin, plog_plugin)

