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

    std::ostream& os;
    mutable std::set<std::string> types;

    void operator()(const polysync::node& top) const {
        eggs::variants::apply([&](auto&& v) { inspect(v); }, top);
    }

    // Skip inspecting most types, because they are native types.
    template <typename T>
    void inspect(const T&) const { }

    // polysync::tree is the nested type that is useful inspect.
    void inspect(const polysync::tree& tree) const {

        if (!descriptor::catalog.count(tree.type)) 
            throw polysync::error("no type description") 
                << exception::type(tree.type)
                << exception::module("datamodel");
                
        const descriptor::type& desc = descriptor::catalog.at(tree.type);
        if (!types.count(tree.type)) {
            // Have not yet seen this type
            types.insert(tree.type);
            os << format.fieldname << format.bold << tree.type << block_begin << wrap << format.normal;
            std::for_each(desc.begin(), desc.end(), [&](const descriptor::field& d) {
                    std::string tname = d.name;
                    std::string tags;
                    if (d.bigendian) 
                        tags += format.note + std::string(" bigendian ") + format.normal;
                    os << tab.back() << format.fieldname << tname << ": " << format.normal 
                       << descriptor::lex(d.type) << format.note 
                       << (tags.empty() ? std::string() : " (" + tags + format.note + ")") 
                       << format.normal << wrap;
                    });
            os << format.fieldname << format.bold << block_end << format.normal << wrap;
        }

        // Recurse all the nested types
        std::for_each(tree->begin(), tree->end(), [&](const node& n) { 
                return eggs::variants::apply([&](auto&& v) { inspect(v); }, n);
            });
    }

    void inspect(const std::vector<polysync::tree>& trees) const {
        // vectors of trees are supposed to be homogeneous, so inspect the first one only.
        if (!trees.empty())
            inspect(trees.front());
    }

    // Customization points for different displays
    mutable std::vector<std::string> tab { "    " };
    std::string wrap { "\n" };
    std::string finish { "" };
    std::string tabstop { "    " };
    std::string sep { "\n" };
    std::string block_begin { " {" };
    std::string block_end { "}" };
};

struct plugin : encode::plugin {

    po::options_description options() const {
        po::options_description opt("Datamodel Options");
        opt.add_options()
            ;
        return opt;
    };

    void connect(const po::variables_map& vm, encode::visitor& visit) const {

        model_printer print { std::cout };

        // Modify the printing style based on input options
        if (vm.count("compact")) {
            print.wrap = "";
            print.sep = ",";
            print.tabstop = " ";
            print.finish = "\n";
        }

        visit.record.connect([print](const plog::log_record& record) { 
                BOOST_LOG_SEV(log, severity::verbose) << record;
                std::istringstream iss(record.blob);
                plog::decoder decode(iss);
                polysync::node top = decode(record);
                print(top);
                std::cout << print.finish;
                });
    }
};

boost::shared_ptr<encode::plugin> create_plugin() {
    return boost::make_shared<plugin>();
}

}}} // namespace polysync::transcode::datamodel

BOOST_DLL_ALIAS(polysync::transcode::datamodel::create_plugin, datamodel_plugin)

