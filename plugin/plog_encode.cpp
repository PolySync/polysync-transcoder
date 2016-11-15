#include <polysync/plugin.hpp>
#include <polysync/logging.hpp>
#include <polysync/plog/encoder.hpp>
#include <boost/make_shared.hpp>
#include <boost/dll/alias.hpp>
#include <boost/filesystem.hpp>

#include <polysync/print_hana.hpp>

namespace polysync {

namespace po = boost::program_options;
namespace fs = boost::filesystem;
namespace hana = boost::hana;

using logging::severity;

static std::ofstream out;
static std::shared_ptr<plog::encoder> encode_;
static logging::logger log { "plog-encode" };

class plog_encoder : public polysync::encode::plugin { 
public:

    po::options_description options() const override {
        po::options_description opt("PLog-Encoder Options");
        opt.add_options()
            ("name,n", po::value<fs::path>(), "filename")
            ;
        return opt;
    }

    void connect(const po::variables_map& vm, encode::visitor& visit) const override {

        std::string path = vm["name"].as<fs::path>().string();

        // Open a new output file for each new decoder opened. Right now, this
        // only works for the first file because there is not yet a scheme to
        // generate unique filenames (FIXME).
        visit.decoder.connect([path](plog::decoder& r) { 
                BOOST_LOG_SEV(log, severity::verbose) << "opening " << path;
                out.open(path, std::ios_base::out | std::ios_base::binary);
                encode_.reset(new polysync::plog::encoder(out));
                });

        // Serialize the global file header.
        visit.log_header.connect([](const plog::log_header& head) {
                encode_->encode(head); 
            });

        // Serialize every record.
        visit.record.connect([](const plog::log_record& record) { 
                BOOST_LOG_SEV(log, severity::verbose) << record;
                std::istringstream iss(record.blob);
                plog::decoder decode(iss);
                polysync::node top = decode(record);
                encode_->encode(top); 
                });
    }
};

extern "C" BOOST_SYMBOL_EXPORT plog_encoder encoder;
plog_encoder encoder;

} // polysync


