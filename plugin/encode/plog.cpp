#include <boost/make_shared.hpp>
#include <boost/dll/alias.hpp>
#include <boost/filesystem.hpp>

#include <polysync/plugin.hpp>
#include <polysync/logging.hpp>
#include <polysync/encoder.hpp>
#include <polysync/print_hana.hpp>

namespace polysync { namespace plugin {

namespace po = boost::program_options;
namespace fs = boost::filesystem;
namespace hana = boost::hana;

using logging::severity;

static std::ofstream out;
static std::shared_ptr<Encoder> writer;
static logging::logger log { "plog" };

using PlogSequencer = Sequencer<plog::ps_log_record>;

class plog_encode : public encode::plugin
{
public:

    po::options_description options() const override {
        po::options_description opt("plog: Encode a new PLog");
        opt.add_options()
            ("outfile,o", po::value<fs::path>()->default_value("/dev/stdout"), "outfile")
            ;
        return opt;
    }

    void connect(const po::variables_map& cmdline_args, encode::Visitor& visit) override {

        std::string path = cmdline_args["outfile"].as<fs::path>().string();

        if ( !descriptor::catalog.count("ps_log_record") )
        {
            throw polysync::error( "missing required type descriptor" )
                << polysync::exception::type( "ps_log_record" );
        }

        // Open a new output file for each new decoder opened. Right now, this
        // only works for the first file because there is not yet a scheme to
        // generate unique filenames (FIXME).
        visit.open.connect([path]( const PlogSequencer& r) {
                BOOST_LOG_SEV(log, severity::verbose) << "opening " << path;
                out.open(path, std::ios_base::out | std::ios_base::binary);
                writer.reset(new Encoder(out));
                });

        // Serialize the global file header.
        visit.log_header.connect([](const plog::ps_log_header& head) {
                writer->encode(head);
            });

        // Serialize every record.
        descriptor::Type desc = descriptor::catalog.at("ps_log_record");
        visit.record.connect([desc](const polysync::Node& top) {
                writer->encode(*top.target<Tree>(), desc);
                });
    }
};

extern "C" BOOST_SYMBOL_EXPORT plog_encode encoder;
plog_encode encoder;

}} // namespace polysync::plugin


