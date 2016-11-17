#include <polysync/plugin.hpp>
#include <polysync/exception.hpp>
#include <boost/filesystem.hpp>
#include <boost/make_shared.hpp>
#include <ctime>

namespace polysync {

namespace po = boost::program_options;
namespace fs = boost::filesystem;

static logging::logger log { "matlab" };
using logging::severity;
using console::format; 

namespace matlab {

#pragma pack(push, 1)
struct header {
    char description[116];
    std::uint64_t subsys_data_offset { 0 };
    std::uint16_t version { 0x0100 };
    char endian_indicator[2] { 'M', 'I' };
};
#pragma pack(pop)

class writer {
    public:
        writer(const fs::path& path) : file(path.string().c_str()) { 
            header head;
            std::stringstream text;
            text.rdbuf()->pubsetbuf(head.description, sizeof(head.description));
            std::time_t time = std::time(nullptr);
            text << "MATLAB 5.0 MAT-file"
                 << ", Platform: Linux"
                 << ", Created on: " << std::asctime(std::localtime(&time));
            file.write((char *)&head, sizeof(header));
        }

        void write(const plog::log_record& record) {
        }

    std::ofstream file;
};

}

std::shared_ptr<matlab::writer> mat;

struct matlab_encoder : polysync::encode::plugin {
    po::options_description options() const override {
        po::options_description opt("Matlab options");
        opt.add_options()
            ("name,n", po::value<fs::path>(), "filename")
            ;   
        return opt;
    }

    void connect(const po::variables_map& vm, encode::visitor& visit) const override {
        if (!vm.count("name"))
            throw polysync::error("missing filename")
                << exception::module("matlab");

        fs::path path = vm["name"].as<fs::path>();
        visit.decoder.connect([path](const plog::decoder& decoder) {
                mat = std::make_shared<matlab::writer>(path);
                });

        visit.record.connect([](const plog::log_record& rec) { mat->write(rec); });
    }
};

boost::shared_ptr<encode::plugin> create_plugin() {
    return boost::make_shared<matlab_encoder>();
}

} // namespace polysync

extern "C" BOOST_SYMBOL_EXPORT polysync::matlab_encoder encoder;
polysync::matlab_encoder encoder;
