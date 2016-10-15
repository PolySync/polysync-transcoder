#include <polysync/transcode/plugin.hpp>
#include <polysync/transcode/io.hpp>
#include <boost/make_shared.hpp>
#include <boost/hana.hpp>

namespace polysync { namespace transcode { namespace dump {

namespace po = boost::program_options;

static logging::logger log { "dump" };
using logging::severity;
using console::format; 
namespace hana = boost::hana;

struct pretty_printer {

    // Pretty print terminal types
    template <typename Number>
    typename std::enable_if_t<!hana::Foldable<Number>::value>
    print(std::ostream& os, const std::string& name, const Number& value) {
        os << tab.back() << format.yellow << name << ": " << format.normal 
           << value << wrap << format.normal;
    }

    // Pretty print boost::hana (static) structures
    template <typename Struct, class = typename std::enable_if_t<hana::Foldable<Struct>::value>>
    void print(std::ostream& os, const std::string& name, const Struct& s) {
        os << tab.back() << format.blue << format.bold << name << " {" << wrap << format.normal;
        tab.push_back(tab.back() + "    ");
        hana::for_each(s, [&os, this](auto f) mutable { 
                print(os, hana::to<char const*>(hana::first(f)), hana::second(f));
                });
        tab.pop_back();
        os << tab.back() << format.blue << format.bold << "}" << format.normal << wrap;
    }

    // Pretty print plog::tree (dynamic) structures
    void print(std::ostream& os, const std::string& name, std::shared_ptr<plog::tree> top) {
        os << tab.back() << format.cyan << format.bold << name << " {" << wrap << format.normal;
        tab.push_back(tab.back() + "    ");
        std::for_each(top->begin(), top->end(), [&](auto pair) { 
                eggs::variants::apply([&](auto f) { print(os, pair.first, f); }, pair.second);
                });
        tab.pop_back();
        os << tab.back() << format.cyan << format.bold << "}" << format.normal << wrap;
    }

    std::shared_ptr<plog::tree> top;
    std::vector<std::string> tab { "" };
    std::string wrap { "\n" };
};

struct plugin : transcode::plugin {

    po::options_description options() const {
        po::options_description opt("Dump Options");
        opt.add_options()
            ("compact", "remove newlines from formatting")
            ;
        return opt;
    };

    void connect(const po::variables_map& vm, transcode::callback& call) const {

        call.record.connect([](const plog::log_record& record) { 
                BOOST_LOG_SEV(log, severity::verbose) << record;
                std::istringstream iss(record.blob);
                plog::dynamic_reader read(iss);
                plog::node top = read();
                pretty_printer pretty;
                pretty.print(std::cout, top.name, *top.target<std::shared_ptr<plog::tree>>());
                });
    }
};

boost::shared_ptr<transcode::plugin> create_plugin() {
    return boost::make_shared<plugin>();
}

}}} // namespace polysync::transcode::dump

BOOST_DLL_ALIAS(polysync::transcode::dump::create_plugin, dump_plugin)

