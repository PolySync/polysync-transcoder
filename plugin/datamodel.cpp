#include <polysync/plugin.hpp>
#include <boost/make_shared.hpp>

// The datamodel plugin dumps the dataypes and the record counts for each type found in a plog.

namespace po = boost::program_options;

namespace polysync { namespace plugin {

static polysync::logging::logger log { "datamodel" };
using polysync::logging::severity;

// model_printer is a visitor on polysync::node; primary useful for recursing polysync::tree.
struct model_printer {

    std::ostream& os;
    mutable std::map<std::string, size_t> types;

    void operator()(const polysync::node& top) const {
        eggs::variants::apply([&](auto&& v) { inspect(v); }, top);
    }

    // Skip inspecting most types, because they are native types like integers and floats.
    template <typename T>
    void inspect(const T&) const { }

    // polysync::tree is the nested type that is useful inspect.
    void inspect(const polysync::tree& tree) const {

        // std::map semantics will create the entry here, with value 0, if key is new.
        types[tree.type] += 1;

        // Recurse all the nested types
        for (const polysync::node& node: *tree)
            eggs::variants::apply([&](auto&& v) { inspect(v); }, node);
    }

    void inspect(const std::vector<polysync::tree>& trees) const {
        // vectors of trees are supposed to be homogeneous, so inspect the first one only.
        if (!trees.empty())
            inspect(trees.front());
    }
};

struct datamodel : encode::plugin {

    po::options_description options() const {
        po::options_description opt("Datamodel Options");
        opt.add_options()
            ("compact", "print type names only")
            ;
        return opt;
    };

    model_printer print { std::cout };

    void connect(const po::variables_map& vm, encode::visitor& visit) const {

        bool compact = vm.count("compact");

        // As each record is visited, we only count them.  Print the result later.
        visit.record.connect([this](const plog::log_record& record) { 
                BOOST_LOG_SEV(log, severity::verbose) << record;
                std::istringstream iss(record.blob);
                plog::decoder decode(iss);
                polysync::node top("log_record", decode(record));
                print(top);
                });

        // At cleanup, we finally print, because now we know how many instances we saw.
        visit.cleanup.connect([this, compact](const plog::decoder& decode) {

                // Iterate all the found types and print them.
                for (auto pair: print.types) {
                    std::string count = "x" + std::to_string(print.types.at(pair.first));

                    if (compact)
                        std::cout << pair.first << " " << count << std::endl;
                    else {
                        std::cout << format->begin_block(pair.first);

                        if (!descriptor::catalog.count(pair.first)) 
                            throw polysync::error("no type description") 
                                << exception::type(pair.first)
                                << status::description_error;

                        for (const descriptor::field& d: descriptor::catalog.at(pair.first)) {
                            std::string tname = d.name;
                            std::string tags;
                            if (d.bigendian) 
                                tags += std::string(" bigendian ");
                            std::cout << format->item(tname, descriptor::lex(d.type), tags);
                        }
                        std::cout << format->end_block(count);
                    }
                    }
                });
    }
};

boost::shared_ptr<encode::plugin> create_datamodel() {
    return boost::make_shared<datamodel>();
}

}} // namespace polysync::plugin

BOOST_DLL_ALIAS(polysync::plugin::create_datamodel, datamodel_plugin)

