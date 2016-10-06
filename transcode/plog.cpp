#include <polysync/transcode/plugin.hpp>
#include <boost/make_shared.hpp>
#include <boost/dll/alias.hpp>
#include <boost/filesystem.hpp>

namespace polysync { namespace transcode { namespace plog {

namespace po = boost::program_options;
namespace fs = boost::filesystem;
namespace hana = boost::hana;
using namespace polysync::plog;

class writer {
public:
    writer(const fs::path& path) : plog(path.string(), std::ios_base::out | std::ios_base::binary) {}

    template <typename Record>
    std::string describe() const {
        std::string result = static_typemap.at(typeid(Record)).name + " { ";
        hana::for_each(Record(), [&result](auto pair) {
                std::type_index tp = typeid(hana::second(pair));
                std::string fieldname = hana::to<char const*>(hana::first(pair));
                if (static_typemap.count(tp) == 0)
                    throw std::runtime_error("type not named for field \"" + fieldname + "\"");
                result += fieldname + ": " + static_typemap.at(tp).name + " " + std::to_string(static_typemap.at(tp).size) +  "; ";
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

