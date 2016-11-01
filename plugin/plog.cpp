#include <polysync/plugin.hpp>
#include <polysync/plog/encoder.hpp>
#include <boost/make_shared.hpp>
#include <boost/dll/alias.hpp>
#include <boost/filesystem.hpp>

#include <polysync/plog/io.hpp>

namespace polysync { namespace transcode { namespace plog {

namespace po = boost::program_options;
namespace fs = boost::filesystem;
namespace hana = boost::hana;
using namespace polysync::plog;

static std::ofstream out;
static std::shared_ptr<polysync::plog::encoder> encode;
static logging::logger log { "plog-encode" };

struct plugin : public encode::plugin { 

    po::options_description options() const {
        po::options_description opt("PLog-Encoder Options");
        opt.add_options()
            ("name,n", po::value<fs::path>(), "filename")
            ;
        return opt;
    }

    void connect(const po::variables_map& vm, encode::visitor& visit) const {

        std::string path = vm["name"].as<fs::path>().string();

        // Open a new output file for each new decoder opened. Right now, this
        // only works for the first file because there is not yet a scheme to
        // generate unique filenames (FIXME).
        visit.decoder.connect([path](plog::decoder& r) { 
                BOOST_LOG_SEV(log, severity::verbose) << "opening " << path;
                out.open(path, std::ios_base::out | std::ios_base::binary);
                encode.reset(new polysync::plog::encoder(out));
                });

        // Serialize the global file header.
        visit.log_header.connect([](const plog::log_header& head) {
                encode->encode(head); 
            });

        // Serialize every record.
        visit.record.connect([](const log_record& record) { 
                BOOST_LOG_SEV(log, severity::verbose) << record;
                std::istringstream iss(record.blob);
                plog::decoder decode(iss);
                plog::node top = decode(record);
                encode->encode(top); 
                });
    }
};

boost::shared_ptr<encode::plugin> create_plugin() {
    return boost::make_shared<struct plugin>();
}

}}} // namespace transcode::plog

BOOST_DLL_ALIAS(polysync::transcode::plog::create_plugin, plog_plugin)

