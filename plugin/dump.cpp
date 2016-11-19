#include <polysync/plugin.hpp>
#include <polysync/exception.hpp>
#include <polysync/print_hana.hpp>
#include <boost/make_shared.hpp>
#include <boost/hana.hpp>
#include <regex>

namespace polysync { namespace transcode { namespace dump {

namespace po = boost::program_options;

namespace formatter = polysync::formatter;
static logging::logger log { "dump" };
using logging::severity;
using console::format; 
namespace hana = boost::hana;

struct pretty_printer {

    void print(std::ostream& os, const std::string& name, tree top) const; 
    void print(std::ostream& os, tree top) const; 

    template <typename T>
        void print(std::ostream& os, const std::string& name, const std::vector<T>& array) const {
            os << "vector1" << std::endl;
            return;
            os << tab.back() << format.tpname << name << ": " << format.normal << wrap;
            std::for_each(array.begin(), array.end(), [&](const T& value) {
                    os << tab.back() << "{" << wrap;
                    tab.push_back(tab.back() + tabstop);
                    os << value;
                    tab.pop_back();
                    os << tab.back() << "}" << wrap;
                    });
        }

    void print(std::ostream& os, const std::string& name, const std::vector<tree>& array) const {
        driver->begin_block(name);
        size_t rec = 0;
        driver->begin_ordered();
        std::for_each(array.begin(), array.end(), [&](const tree& value) { 
                rec += 1;
                print(os, value); 
                });
        driver->end_ordered();
        driver->end_block();
    }

    // Pretty print terminal types
    template <typename Number>
        typename std::enable_if_t<!hana::Foldable<Number>::value>
        print(std::ostream& os, const std::string& name, const Number& value) const {
            std::stringstream ss;
            ss << value;
            std::string type = descriptor::typemap.at(typeid(Number)).name;
            driver->item(name, ss.str(), type);
        }

    // Print chars as integers
    void print(std::ostream& os, const std::string& name, const std::uint8_t& value) const {
        std::stringstream ss;
        ss << static_cast<std::uint16_t>(value);
        driver->item(name, ss.str(), "uint16");
    }

    // Pretty print boost::hana (static) structures
    template <typename Struct, class = typename std::enable_if_t<hana::Foldable<Struct>::value>>
        void print(std::ostream& os, const std::string& name, const Struct& s) const {
            os << "hana" << std::endl;
            return;
            os << tab.back() << format.tpname << format.bold << name << " {" << wrap << format.normal;
            tab.push_back(tab.back() + "    ");
            hana::for_each(s, [&os, this](auto f) mutable { 
                    print(os, hana::to<char const*>(hana::first(f)), hana::second(f));
                    });
            tab.pop_back();
            os << tab.back() << format.tpname << format.bold << "}" << format.normal << wrap;
        }

    // Pretty print tree (dynamic) structures
    mutable std::vector<std::string> tab { "" };
    std::string wrap { "\n" };
    std::string finish { "" };
    std::string tabstop { "    " };
    std::string sep { "\n" };

    std::shared_ptr<formatter::interface> driver;

};

void pretty_printer::print( std::ostream& os, const std::string& name, tree top ) const {
    driver->begin_block(name);
    std::for_each(top->begin(), top->end(), [&](auto pair) { 
            eggs::variants::apply([&](auto f) { print(os, pair.name, f); }, pair);
            });
    driver->end_block();
}

void pretty_printer::print(std::ostream& os, tree top) const {
    driver->begin_block(top.type);
    std::for_each(top->begin(), top->end(), 
            [&](auto pair) { 
            eggs::variants::apply([&](auto f) { print(os, pair.name, f); }, pair);
            });
    driver->end_block();
}


// Specialize bytes so it does not print characters, which is useless behavior.
template <>
void pretty_printer::print(std::ostream& os, const std::string& name, const bytes& record) const {
    std::stringstream ss;
    const int psize = 12;
    ss << "[ " << std::hex;
    std::for_each(record.begin(), std::min(record.begin() + psize, record.end()), 
            [&ss](auto field) mutable { ss << ((std::uint16_t)field & 0xFF) << " "; });

    if (record.size() > 2*psize)
        ss << "... ";
    if (record.size() > psize)
        std::for_each(record.end() - psize, record.end(), 
                [&ss](auto field) mutable { ss << ((std::uint16_t)field & 0xFF) << " "; });

    ss << "]" << std::dec << " (" << record.size() << " elements)";
    driver->item(name, ss.str(), "");
}

struct plugin : encode::plugin {

    po::options_description options() const {
        po::options_description opt("Dump Options");
        opt.add_options()
            ("compact", "remove newlines from formatting")
            ("markdown", "format as Markdown")
            ("type", "show types along with values")
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

        if (vm.count("markdown")) 
            pretty.driver = std::shared_ptr<formatter::interface>(new formatter::markdown( std::cout ));
        else
            pretty.driver = std::shared_ptr<formatter::interface>(new formatter::console( std::cout ));
        pretty.driver->show_type = vm.count("type");

        visit.record.connect([pretty](const plog::log_record& record) { 
                BOOST_LOG_SEV(log, severity::verbose) << record;
                std::istringstream iss(record.blob);
                plog::decoder decode(iss);
                node top = decode(record);
                pretty.print(std::cout, top.name, *top.target<tree>());
                });
    }
};

boost::shared_ptr<encode::plugin> create_plugin() {
    return boost::make_shared<plugin>();
}

}}} // namespace polysync::transcode::dump

BOOST_DLL_ALIAS(polysync::transcode::dump::create_plugin, dump_plugin)

