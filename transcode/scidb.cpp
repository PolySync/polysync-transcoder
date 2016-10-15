#include <polysync/transcode/plugin.hpp>

namespace po = boost::program_options;

namespace polysync { namespace transcode { namespace scidb {

struct plugin : transcode::plugin { 

    po::options_description options() const {
        po::options_description opt("SciDB Options");
        return opt;
    }

    void connect(const po::variables_map& vm, callback& call) const {
    }

};

extern "C" BOOST_SYMBOL_EXPORT plugin plugin;

struct plugin plugin;

}}} // namespace polysync::transcode::scidb


