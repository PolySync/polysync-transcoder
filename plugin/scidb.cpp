#include <polysync/plugin.hpp>

namespace po = boost::program_options;

namespace polysync { namespace transcode { namespace scidb {

struct plugin : encode::plugin { 

    po::options_description options() const {
        po::options_description opt("SciDB Options");
        return opt;
    }

    void connect(const po::variables_map& vm, encode::visitor& visit) const {
    }

};

extern "C" BOOST_SYMBOL_EXPORT plugin plugin;

struct plugin plugin;

}}} // namespace polysync::transcode::scidb


