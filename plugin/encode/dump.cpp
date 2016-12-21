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
    std::ostream& os;

    // Entry point from visitor callback
    void operator()(const node& top) const { print (top, *top.target<tree>()); }

    void print(const node& n, tree top) const;
    void print(tree top) const;

    void print(const node& n, const boost::multiprecision::cpp_int& value) const {
        os << std::hex << value << std::dec;
    }

    template <typename T>
    void print(const node& n, const std::vector<T>& array) const {
        os << format->begin_block(n.name);
        std::for_each(array.begin(), array.end(), [this](const T& v) { os << v; });
        os << format->end_block();
    }

    void print(const polysync::node& node, const std::vector<tree>& array) const {
        os << format->begin_block(node.name);
        size_t rec = 0;
        for (const tree& value: array) {
            rec += 1;
            os << format->begin_ordered(rec, value.type);
            print(value);
            os << format->end_ordered();
        };
        os << format->end_block();
    }

    // Pretty print terminal types
    template <typename Number>
    typename std::enable_if_t<std::is_arithmetic<Number>::value>
    print(const node& n, const Number& value) const {
        std::stringstream ss;
        if (n.format)
            ss << n.format(n);
        else
            ss << value;
        std::string type = descriptor::terminalTypeMap.at(typeid(Number)).name;
        os << format->item(n.name, ss.str(), type);
    }

    // Specialize chars so they print as integers
    void print(const node& n, const std::uint8_t& value) const {
        std::stringstream ss;
        ss << static_cast<std::uint16_t>(value);
        os << format->item(n.name, ss.str(), "uint16");
    }

};

// Specialize bytes so it does not print characters, which is useless behavior.
template <>
void pretty_printer::print(const node& n, const bytes& record) const {
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
    os << format->item(n.name, ss.str(), "");
}

void pretty_printer::print( const node& n, tree top ) const {
    os << format->begin_block(n.name);
    for (const polysync::node& node: *top)
        eggs::variants::apply([this, &node](auto& f) { this->print(node, f); }, node);
    os << format->end_block();
}

void pretty_printer::print( tree top ) const {
    format->begin_block(top.type);
    std::for_each(top->begin(), top->end(),
            [this](auto& pair) {
            eggs::variants::apply([this, &pair](auto& f) { this->print(pair, f); }, pair);
            });
    format->end_block();
}


struct dump : encode::plugin {

    po::options_description options() const override {
        po::options_description opt("dump: Display file contents");
        opt.add_options()
            ;
        return opt;
    };

    void connect(const po::variables_map& cmdline_args, encode::visitor& visit) override {
        visit.record.connect(pretty_printer { std::cout } );
    }
};

boost::shared_ptr<encode::plugin> create_dump() {
    return boost::make_shared<dump>();
}

}} // namespace polysync::plugin

BOOST_DLL_ALIAS(polysync::plugin::create_dump, dump_plugin)

