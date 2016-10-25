#include <polysync/transcode/plugin.hpp>
#include <polysync/transcode/io.hpp>
#include <boost/make_shared.hpp>
#include <boost/hana.hpp>
#include <regex>

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
    print(std::ostream& os, const std::string& name, const Number& value) const {
        os << tab.back() << format.green << name << ": " << format.normal 
           << value << sep << format.normal;
    }

    // Pretty print boost::hana (static) structures
    template <typename Struct, class = typename std::enable_if_t<hana::Foldable<Struct>::value>>
    void print(std::ostream& os, const std::string& name, const Struct& s) const {
        os << tab.back() << format.blue << format.bold << name << " {" << wrap << format.normal;
        tab.push_back(tab.back() + "    ");
        hana::for_each(s, [&os, this](auto f) mutable { 
                print(os, hana::to<char const*>(hana::first(f)), hana::second(f));
                });
        tab.pop_back();
        os << tab.back() << format.blue << format.bold << "}" << format.normal << wrap;
    }

    // Pretty print plog::tree (dynamic) structures
    void print(std::ostream& os, const std::string& name, std::shared_ptr<plog::tree> top) const {
        os << tab.back() << format.cyan << format.bold << name << " {" << wrap << format.normal;
        tab.push_back(tab.back() + tabstop);
        std::for_each(top->begin(), top->end(), 
                [&](auto pair) { 
                eggs::variants::apply([&](auto f) { print(os, pair.name, f); }, pair);
                });
        tab.pop_back();
        os << tab.back() << format.cyan << format.bold << "}" << format.normal << wrap;
    }

    mutable std::vector<std::string> tab { "" };
    std::string wrap { "\n" };
    std::string finish { "" };
    std::string tabstop { "    " };
    std::string sep { "\n" };
};

struct plugin : transcode::plugin {

    po::options_description options() const {
        po::options_description opt("Dump Options");
        opt.add_options()
            ("compact", "remove newlines from formatting")
            ;
        return opt;
    };

    void connect(const po::variables_map& vm, transcode::visitor& visit) const {

        pretty_printer pretty;
        if (vm.count("compact")) {
            pretty.wrap = "";
            pretty.sep = ",";
            pretty.tabstop = " ";
            pretty.finish = "\n";
        }

        visit.record.connect([pretty](const plog::log_record& record) { 
                BOOST_LOG_SEV(log, severity::verbose) << record;
                std::istringstream iss(record.blob);
                plog::dynamic_decoder read(iss);
                plog::node top = read.decode(record);
                pretty.print(std::cout, top.name, *top.target<std::shared_ptr<plog::tree>>());
                std::cout << pretty.finish;
                });
    }
};

boost::shared_ptr<transcode::plugin> create_plugin() {
    return boost::make_shared<plugin>();
}

}}} // namespace polysync::transcode::dump

BOOST_DLL_ALIAS(polysync::transcode::dump::create_plugin, dump_plugin)

