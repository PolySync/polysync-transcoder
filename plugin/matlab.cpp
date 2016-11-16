#include <polysync/plugin.hpp>
#include <boost/filesystem.hpp>
#include <boost/make_shared.hpp>

namespace polysync {

namespace po = boost::program_options;
namespace fs = boost::filesystem;

static logging::logger log { "matlab" };
using logging::severity;
using console::format; 

struct matlab_encoder : polysync::encode::plugin {
    po::options_description options() const override {
        po::options_description opt("Matlab options");
        opt.add_options()
            ;   
        return opt;
    }

    void connect(const po::variables_map& vm, encode::visitor& visit) const override {
    }
};

boost::shared_ptr<encode::plugin> create_plugin() {
    return boost::make_shared<matlab_encoder>();
}

} // namespace polysync

extern "C" BOOST_SYMBOL_EXPORT polysync::matlab_encoder encoder;
polysync::matlab_encoder encoder;
