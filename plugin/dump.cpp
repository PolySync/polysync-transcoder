#include <polysync/plugin.hpp>
#include <polysync/exception.hpp>
#include <polysync/io.hpp>
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

    void print(std::ostream& os, const std::string& name, plog::tree top) const; 
    void print(std::ostream& os, plog::tree top) const; 

    template <typename T>
    void print(std::ostream& os, const std::string& name, const std::vector<T>& array) const {
        os << tab.back() << format.green << name << ": " << format.normal << wrap;
        std::for_each(array.begin(), array.end(), [&](const T& value) {
                os << tab.back() << "{" << wrap;
                tab.push_back(tab.back() + tabstop);
                os << value;
                tab.pop_back();
                os << tab.back() << "}" << wrap;
                });
    }

    void print(std::ostream& os, const std::string& name, const std::vector<plog::tree>& array) const {
        os << tab.back() << format.green << name << ": " << format.normal << wrap;
        size_t rec = 0;
        std::for_each(array.begin(), array.end(), [&](const plog::tree& value) { 
                rec += 1;
                os << tab.back() << rec << ": ";
                print(os, value); 
                });
        os << wrap;
    }

    // Pretty print terminal types
    template <typename Number>
    typename std::enable_if_t<!hana::Foldable<Number>::value>
    print(std::ostream& os, const std::string& name, const Number& value) const {
        os << tab.back() << format.green << name << ": " << format.normal 
         << value << sep << format.normal;
    }

    // Print chars as integers
    void print(std::ostream& os, const std::string& name, const std::uint8_t& value) const {
        os << tab.back() << format.green << name << ": " << format.normal 
         << (std::uint16_t)value << sep << format.normal;
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
    mutable std::vector<std::string> tab { "" };
    std::string wrap { "\n" };
    std::string finish { "" };
    std::string tabstop { "    " };
    std::string sep { "\n" };
};

void pretty_printer::print(std::ostream& os, const std::string& name, plog::tree top) const {
        os << tab.back() << format.cyan << format.bold << name << " {" << wrap << format.normal;
        tab.push_back(tab.back() + tabstop);
        std::for_each(top->begin(), top->end(), 
                [&](auto pair) { 
                eggs::variants::apply([&](auto f) { print(os, pair.name, f); }, pair);
                });
        tab.pop_back();
        os << tab.back() << format.cyan << format.bold << "}" << format.normal << wrap;
    }

void pretty_printer::print(std::ostream& os, plog::tree top) const {
        os << format.cyan << format.bold << "{" << wrap << format.normal;
        tab.push_back(tab.back() + tabstop);
        std::for_each(top->begin(), top->end(), 
                [&](auto pair) { 
                eggs::variants::apply([&](auto f) { print(os, pair.name, f); }, pair);
                });
        tab.pop_back();
        os << tab.back() << format.cyan << format.bold << "}" << format.normal << wrap;
    }


struct plugin : encode::plugin {

    po::options_description options() const {
        po::options_description opt("Dump Options");
        opt.add_options()
            ("compact", "remove newlines from formatting")
            ;
        return opt;
    };

    void connect(const po::variables_map& vm, encode::visitor& visit) const {

        pretty_printer pretty;

        // Modify the printing style based on input options
        if (vm.count("compact")) {
            pretty.wrap = "";
            pretty.sep = ",";
            pretty.tabstop = " ";
            pretty.finish = "\n";
        }

        visit.record.connect([pretty](const plog::log_record& record) { 
                BOOST_LOG_SEV(log, severity::verbose) << record;
                std::istringstream iss(record.blob);
                plog::decoder decode(iss);
                plog::node top = decode(record);
                pretty.print(std::cout, top.name, *top.target<plog::tree>());
                std::cout << pretty.finish;
                });
    }
};

boost::shared_ptr<encode::plugin> create_plugin() {
    return boost::make_shared<plugin>();
}

}}} // namespace polysync::transcode::dump

BOOST_DLL_ALIAS(polysync::transcode::dump::create_plugin, dump_plugin)

