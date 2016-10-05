#include <polysync/transcode/plugin.hpp>
#include <boost/make_shared.hpp>
#include <boost/dll/alias.hpp>
#include <boost/filesystem.hpp>
#include <typeindex>

namespace polysync { namespace transcode { namespace plog {

namespace po = boost::program_options;
namespace fs = boost::filesystem;
namespace hana = boost::hana;
using namespace polysync::plog;

const std::map<std::type_index, std::string> typemap {
    { std::type_index(typeid(std::uint8_t)), "uint8" },
    { std::type_index(typeid(std::uint16_t)), "uint16" },
    { std::type_index(typeid(std::uint32_t)), "uint32" },
    { std::type_index(typeid(std::uint64_t)), "uint64" },
    { std::type_index(typeid(log_module)), "log_module" },
    { std::type_index(typeid(type_support)), "type_support" },
    { std::type_index(typeid(log_header)), "log_header" },
    { std::type_index(typeid(log_record)), "log_record" },
    { std::type_index(typeid(byte_array)), "byte_array" },
    { std::type_index(typeid(sequence<std::uint32_t, log_module>)), "sequence<log_module>" },
    { std::type_index(typeid(sequence<std::uint32_t, type_support>)), "sequence<type_support>" }
};

class writer {
public:
    writer(const fs::path& path) : plog(path.string(), std::ios_base::out | std::ios_base::binary) {}

    template <typename Record>
    std::string describe() const {
        std::string result = typemap.at(typeid(Record)) + " { ";
        hana::for_each(Record(), [&result](auto pair) {
                std::type_index tp = typeid(hana::second(pair));
                std::string fieldname = hana::to<char const*>(hana::first(pair));
                if (typemap.count(tp) == 0)
                    throw std::runtime_error("type not named for field \"" + fieldname + "\"");
                result += fieldname + ": " + typemap.at(tp) + "; ";
                });
        return result + "}";
    }

    // For flat objects like arithmetic types, just straight copy memory as blob to file
    template <typename Number>
    typename std::enable_if_t<!hana::Foldable<Number>::value>
    write(Number& record) {
        plog.write(reinterpret_cast<char *>(&record), sizeof(Number)); 
    }
    
    // Specialize write() for hana wrapped structures.  This works out packing
    // the structure where a straight memcpy would fail due to padding, and also
    // recurses into nested structures.
    template <typename Record>
    void write(const Record& rec) {
        hana::for_each(rec, [this](auto pair) { write(hana::second(pair)); });
    }
    
    // Sequences have a LenType number first specifying how many T's follow.
    template <typename LenType, typename T> 
    void write(sequence<LenType, T>& seq) {
        LenType len = seq.size();
        plog.write(reinterpret_cast<char *>(&len), sizeof(LenType));
        std::for_each(seq.begin(), seq.end(), [this](auto val) { write(val); });
    }

    // name_type is a specialized sequence that is like a Pascal string (length
    // first, no trailing zero as a C string would have)
    void write(name_type& name) {
        std::uint16_t len = name.size();
        plog.write((char *)(name.data()), len); 
    }


    std::ofstream plog;

};
 
struct plugin : public transcode::plugin { 

    po::options_description options() const {
        po::options_description opt("PLog Options");
        opt.add_options()
            ("name,n", po::value<fs::path>(), "filename")
            ;
        return opt;
    }

    void observe(const po::variables_map& vm, callback& call) const {
        using reader = polysync::plog::reader;
        auto w = std::make_shared<writer>(vm["name"].as<fs::path>());

        call.reader.connect([w](const reader& r) { 
                w->write(r.get_header()); 
                w->describe<log_header>();
                });

                
    }

    std::ofstream target;

};

boost::shared_ptr<transcode::plugin> create_plugin() {
    return boost::make_shared<struct plugin>();
}

}}} // namespace transcode::plog

BOOST_DLL_ALIAS(polysync::transcode::plog::create_plugin, plog_plugin)

