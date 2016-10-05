#include <polysync/transcode/plugin.hpp>
#include <boost/make_shared.hpp>
#include <boost/dll/alias.hpp>

namespace polysync { namespace transcode { namespace csv {

namespace po = boost::program_options;

struct plugin : public transcode::plugin { 

    po::options_description options() const {
        po::options_description opt("CSV Options");
        return opt;
    }

    void observe(const po::variables_map& vm, callback& call) const {
    }

};

boost::shared_ptr<transcode::plugin> create_plugin() {
    return boost::make_shared<struct plugin>();
}

}}} // namespace transcode::csv

BOOST_DLL_ALIAS(polysync::transcode::csv::create_plugin, csv_plugin)

