#include <polysync/transcode/plugin.hpp>
#include <hdf5.h>

namespace po = boost::program_options;
namespace fs = boost::filesystem;

namespace polysync { namespace transcode { namespace hdf5 {

// Use the standard C interface for HDF5; I think the C++ interface is 90's
// vintage C++ which is still basically C and just a thin and useless wrapper.

class writer {
public:
    writer(const fs::path& path) {
        file = H5Fcreate(path.string().c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
    }

    void close() {
        std::cout << "close";
        H5Fclose(file);
    }

    hid_t file;
};

struct plugin : transcode::plugin { 

    po::options_description options() const {
        po::options_description opt("HDF5 Options");
        opt.add_options()
            ("name,n", po::value<fs::path>(), "filename")
            ;
        return opt;
    }

    void observe(const po::variables_map& vm, callback& call) const {

        auto w = std::make_shared<writer>(vm["name"].as<fs::path>());
        call.reader.connect([w](const plog::reader& r) {
                });

        call.cleanup.connect([w](const plog::reader&) { w->close(); });
    }

};

extern "C" BOOST_SYMBOL_EXPORT plugin plugin;

struct plugin plugin;

}}} // namespace polysync::transcode::hdf5


