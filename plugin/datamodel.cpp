#include <polysync/plugin.hpp>
#include <polysync/print_hana.hpp>
#include <boost/make_shared.hpp>
#include <boost/hana.hpp>
#include <regex>

namespace po = boost::program_options;

namespace polysync { namespace transcode { namespace datamodel {

static logging::logger log { "datamodel" };
using logging::severity;
using console::format; 
namespace hana = boost::hana;

struct model_printer {

    mutable std::set<std::string> catalog;

    // Pretty print plog::tree (dynamic) structures
    void operator()(std::ostream& os, const std::string& name, const polysync::node& node) const {
        if (node.target_type() == typeid(polysync::tree)) {
            polysync::tree top = *node.target<polysync::tree>();
            if (!polysync::descriptor::catalog.count(name)) 
                throw polysync::error("no description") << exception::type(name);
                
            const polysync::descriptor::type& desc = polysync::descriptor::catalog.at(name);
            if (!catalog.count(name)) 
            {
                catalog.insert(name);
                os << format.fieldname << format.bold << name << " {" << wrap << format.normal;
                std::for_each(desc.begin(), desc.end(), [&](const polysync::descriptor::field& d) {
                        std::string tname = d.name;
                        std::string tags;
                        if (tname.front() == '>') {
                            tags += format.note + std::string(" bigendian ") + format.normal;
                            tname.erase(tname.begin());
                        }
                        os << tab.back() << format.fieldname << tname << ": " << format.normal << d.name 
                        << format.note << (tags.empty() ? std::string() : " (" + tags + format.note + ")") << format.normal
                        << wrap;
                        });
                os << format.fieldname << format.bold << "}" << format.normal << wrap;
            }
            std::for_each(top->begin(), top->end(), [&](const polysync::node& n) { 
                    operator()(os, n.name, n);
                });
        }
    }

    mutable std::vector<std::string> tab { "    " };
    std::string wrap { "\n" };
    std::string finish { "" };
    std::string tabstop { "    " };
    std::string sep { "\n" };
};

struct plugin : encode::plugin {

    po::options_description options() const {
        po::options_description opt("Datamodel Options");
        opt.add_options()
            ;
        return opt;
    };

    void connect(const po::variables_map& vm, encode::visitor& visit) const {

        model_printer printer;

        // Modify the printing style based on input options
        if (vm.count("compact")) {
            printer.wrap = "";
            printer.sep = ",";
            printer.tabstop = " ";
            printer.finish = "\n";
        }

        visit.record.connect([printer](const plog::log_record& record) { 
                BOOST_LOG_SEV(log, severity::verbose) << record;
                std::istringstream iss(record.blob);
                plog::decoder decode(iss);
                polysync::node top = decode(record);
                printer(std::cout, top.name, top); // *top.target<polysync::tree>());
                std::cout << printer.finish;
                });
    }
};

boost::shared_ptr<encode::plugin> create_plugin() {
    return boost::make_shared<plugin>();
}

}}} // namespace polysync::transcode::datamodel

BOOST_DLL_ALIAS(polysync::transcode::datamodel::create_plugin, datamodel_plugin)

