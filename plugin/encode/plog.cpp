#include <polysync/plugin.hpp>
#include <polysync/logging.hpp>
#include <polysync/plog/encoder.hpp>
#include <boost/make_shared.hpp>
#include <boost/dll/alias.hpp>
#include <boost/filesystem.hpp>

#include <polysync/print_hana.hpp>

namespace polysync { namespace plugin {

namespace po = boost::program_options;
namespace fs = boost::filesystem;
namespace hana = boost::hana;

using logging::severity;

static std::ofstream out;
static std::shared_ptr<plog::encoder> writer;
static logging::logger log { "plog" };

class plog_encode : public encode::plugin { 
public:

    po::options_description options() const override {
        po::options_description opt("plog: Encode a new PLog");
        opt.add_options()
            ("outfile,o", po::value<fs::path>()->default_value("/dev/stdout"), "outfile")
            ;
        return opt;
    }

    void connect(const po::variables_map& cmdline_args, encode::visitor& visit) override {

        std::string path = cmdline_args["outfile"].as<fs::path>().string();

        // Open a new output file for each new decoder opened. Right now, this
        // only works for the first file because there is not yet a scheme to
        // generate unique filenames (FIXME).
        visit.open.connect([path](plog::decoder& r) { 
                BOOST_LOG_SEV(log, severity::verbose) << "opening " << path;
                out.open(path, std::ios_base::out | std::ios_base::binary);
                writer.reset(new plog::encoder(out));
                });

        // Serialize the global file header.
        visit.log_header.connect([](const plog::log_header& head) {
                writer->encode(head); 
            });

        // Serialize every record.
        descriptor::type desc = descriptor::catalog.at("log_record");
        visit.record.connect([desc](const polysync::node& top) { 
                writer->encode(*top.target<tree>(), desc); 
                });
    } 
};

extern "C" BOOST_SYMBOL_EXPORT plog_encode encoder;
plog_encode encoder;

}} // namespace polysync::plugin


