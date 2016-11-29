#include <polysync/plugin.hpp>
#include <polysync/exception.hpp>
#include <polysync/print_hana.hpp>
#include <boost/make_shared.hpp>
#include <boost/hana.hpp>
#include <regex>

namespace polysync { namespace plugin {

namespace po = boost::program_options;

static logging::logger log { "dump" };
using logging::severity;

struct pretty_printer {

    void print(std::ostream& os, const node& n, tree top) const; 
    void print(std::ostream& os, tree top) const; 

    template <typename T>
    void print(std::ostream& os, const node& n, const std::vector<T>& array) const {
        os << format->begin_block(n.name);
        for (const T& value: array) {
            os << value; // FIXME
        }
        os << format->end_block();
    }

    void print(std::ostream& os, const polysync::node& node, const std::vector<tree>& array) const {
        os << format->begin_block(node.name);
        size_t rec = 0;
        for (const tree& value: array) {
            rec += 1;
            os << format->begin_ordered(rec, value.type);
            print(os, value); 
            os << format->end_ordered();
        };
        os << format->end_block();
    }

    // Pretty print terminal types
    template <typename Number>
    typename std::enable_if_t<std::is_arithmetic<Number>::value>
    print(std::ostream& os, const node& n, const Number& value) const {
        std::stringstream ss;
        ss << value;
        std::string type = descriptor::typemap.at(typeid(Number)).name;
        os << format->item(n.name, ss.str(), type);
    }

    // Print chars as integers
    void print(std::ostream& os, const node& n, const std::uint8_t& value) const {
        std::stringstream ss;
        ss << static_cast<std::uint16_t>(value);
        os << format->item(n.name, ss.str(), "uint16");
    }
};

// Specialize bytes so it does not print characters, which is useless behavior.
template <>
void pretty_printer::print(std::ostream& os, const node& n, const bytes& record) const {
    std::stringstream ss;
    const int psize = 12;
    ss << "[ " << std::hex;
    std::for_each(record.begin(), std::min(record.begin() + psize, record.end()), 
            [&ss](auto& field) mutable { ss << ((std::uint16_t)field & 0xFF) << " "; });

    if (record.size() > 2*psize)
        ss << "... ";
    if (record.size() > psize)
        std::for_each(record.end() - psize, record.end(), 
                [&ss](auto& field) mutable { ss << ((std::uint16_t)field & 0xFF) << " "; });

    ss << "]" << std::dec << " (" << record.size() << " elements)";
    format->item(n.name, ss.str(), "");
}

void pretty_printer::print( std::ostream& os, const node& n, tree top ) const {
    os << format->begin_block(n.name);
    for (const polysync::node& node: *top) 
        eggs::variants::apply([&](auto& f) { print(os, node, f); }, node);
    os << format->end_block();
}

void pretty_printer::print( std::ostream& os, tree top ) const {
    format->begin_block(top.type);
    std::for_each(top->begin(), top->end(), 
            [&](auto& pair) { 
            eggs::variants::apply([&](auto& f) { print(os, pair, f); }, pair);
            });
    format->end_block();
}


struct dump : encode::plugin {

    po::options_description options() const {
        po::options_description opt("Dump Options");
        opt.add_options()
            ;
        return opt;
    };

    void connect(const po::variables_map& vm, encode::visitor& visit) const {

        pretty_printer pretty;

        visit.record.connect([pretty](const plog::log_record& record) { 
                BOOST_LOG_SEV(log, severity::verbose) << record;
                std::istringstream iss(record.blob);
                plog::decoder decode(iss);
                node top("log_record", decode(record));
                pretty.print(std::cout, top, *top.target<tree>());
                });
    }
};

boost::shared_ptr<encode::plugin> create_dump() {
    return boost::make_shared<dump>();
}

}} // namespace polysync::plugin

BOOST_DLL_ALIAS(polysync::plugin::create_dump, dump_plugin)

