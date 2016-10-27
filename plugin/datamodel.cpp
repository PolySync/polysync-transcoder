#include <polysync/transcode/plugin.hpp>
#include <polysync/plog/io.hpp>
#include <boost/make_shared.hpp>
#include <boost/hana.hpp>
#include <regex>

namespace polysync { namespace transcode { namespace datamodel {

namespace po = boost::program_options;

static logging::logger log { "datamodel" };
using logging::severity;
using console::format; 
namespace hana = boost::hana;

struct model_printer {

    mutable std::set<std::string> catalog;

    // Pretty print plog::tree (dynamic) structures
    void operator()(std::ostream& os, const std::string& name, const plog::node& node) const {
        if (node.target_type() == typeid(plog::tree)) {
            plog::tree top = *node.target<plog::tree>();
            if (!plog::descriptor::catalog.count(name)) 
                throw std::runtime_error("non-described type \"" + name + "\"");
                
            const plog::descriptor::type& desc = plog::descriptor::catalog.at(name);
            if (!catalog.count(name)) 
            {
                catalog.insert(name);
                os << format.cyan << format.bold << name << " {" << wrap << format.normal;
                std::for_each(desc.begin(), desc.end(), [&](const plog::descriptor::field& d) {
                        std::string tname = d.type;
                        std::string tags;
                        if (tname.front() == '>') {
                            tags += format.yellow + std::string(" bigendian ") + format.normal;
                            tname.erase(tname.begin());
                        }
                        os << tab.back() << format.green << tname << ": " << format.normal << d.name 
                        << format.green << (tags.empty() ? std::string() : " (" + tags + format.green + ")") << format.normal
                        << wrap;
                        });
                os << format.cyan << format.bold << "}" << format.normal << wrap;
            }
            std::for_each(top->begin(), top->end(), [&](const plog::node& n) { 
                    operator()(os, n.type, n);
                });
        }
    }

    mutable std::vector<std::string> tab { "    " };
    std::string wrap { "\n" };
    std::string finish { "" };
    std::string tabstop { "    " };
    std::string sep { "\n" };
};

struct plugin : transcode::plugin {

    po::options_description options() const {
        po::options_description opt("Datamodel Options");
        opt.add_options()
            ;
        return opt;
    };

    void connect(const po::variables_map& vm, transcode::visitor& visit) const {

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
                plog::node top = decode(record);
                printer(std::cout, top.name, *top.target<plog::tree>());
                std::cout << printer.finish;
                });
    }
};

boost::shared_ptr<transcode::plugin> create_plugin() {
    return boost::make_shared<plugin>();
}

}}} // namespace polysync::transcode::datamodel

BOOST_DLL_ALIAS(polysync::transcode::datamodel::create_plugin, datamodel_plugin)

