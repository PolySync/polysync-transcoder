#include <polysync/transcode/plugin.hpp>
#include <polysync/transcode/io.hpp>
#include <polysync/transcode/dynamic.hpp>
#include <boost/make_shared.hpp>
#include <boost/dll/alias.hpp>

namespace polysync { namespace transcode { namespace query {

namespace po = boost::program_options;
namespace endian = boost::endian;

static std::map<plog::ps_msg_type, std::string> msg_type;

void parse_description(const std::string& msg_type, std::istream& iss) {
    plog::reader r(iss);
    std::cout << msg_type << " inpos " << iss.tellg() << std::endl;
    const plog::type_descriptor& desc = plog::description_map.at(msg_type);
    std::for_each(desc.begin(), desc.end(), [&r](auto field) {
            plog::atom atom;
            if (field.type == "log_record") {
                std::cout << "in log_record" << std::endl;
                atom = (std::uint32_t)0;
            }
            if (field.type == "log_header") {
                std::cout << "in log_header" << std::endl;
                atom = (std::uint32_t)0;
            }
            if (field.type == "sequence<octet>") {
            std::cout << "in sequence" << std::endl;
                auto seq = r.read<plog::sequence<std::uint32_t, std::uint8_t>>();
                std::cout << field << " = " <<  seq << std::endl;
                std::cout << std::hex;
                std::istringstream sub(seq);
                std::cout << "pos " << sub.tellg() << std::endl;
                parse_description("ibeo.header", sub);
                std::cout << std::dec;
                std::istringstream sub2(std::string(seq.begin() + sub.tellg(), seq.end()));
                parse_description("ibeo.vehicle_state", sub2); 
                std::cout << std::dec;
                atom = (std::uint32_t)0;
            }
            if (field.type == "uint8")
                atom = r.read<std::uint8_t>();
            if (field.type == "uint16")
                atom = r.read<std::uint16_t>();
            if (field.type == "uint32")
                atom = r.read<std::uint32_t>();
            if (field.type == "uint64")
                atom = r.read<std::uint64_t>();
            if (field.type == "int8")
                atom = r.read<std::int8_t>();
            if (field.type == "int16")
                atom = r.read<std::int16_t>();
            if (field.type == "int32")
                atom = r.read<std::int32_t>();
            if (field.type == "int64")
                atom = r.read<std::uint64_t>();
            if (field.type == ">int16")
                atom = r.read<endian::big_int16_buf_t>();
            if (field.type == ">int32")
                atom = r.read<endian::big_int32_buf_t>();
            if (field.type == ">int64")
                atom = r.read<endian::big_int64_buf_t>();
            if (field.type == ">uint16")
                atom = r.read<endian::big_uint16_buf_t>();
            if (field.type == ">uint32")
                atom = r.read<endian::big_uint32_buf_t>();
            if (field.type == ">uint64")
                atom = r.read<endian::big_uint64_buf_t>();
            if (field.type == ">NTP64")
                atom = r.read<endian::big_uint64_buf_t>();
            if (field.type == "ps_guid")
                atom = r.read<std::uint64_t>();
            if (field.type == "ps_timestamp")
                atom = r.read<std::uint64_t>();

            if (!atom)
                throw std::runtime_error("no reader for atom " + field.type);

            std::cout << field << " = ";
            eggs::variants::apply([](auto&& a) { std::cout << a << std::endl; }, atom);

    });
    std::cout << std::endl;
}

struct plugin : transcode::plugin { 

    po::options_description options() const {
        po::options_description opt("Query Options");
        opt.add_options()
            ("version", "dump type support to stdout")
            ("types", "dump type support to stdout")
            ("modules", "dump modules to stdout")
            ("count", "print number of records to stdout")
            ("headers", "dump header summaries to stdout")
            ("dump", "dump entire record to stdout")
            ;

        return opt;
    }

    void observe(const po::variables_map& vm, callback& call) const {
        call.type_support.connect([](plog::type_support t) { msg_type.emplace(t.type, t.name); });

        // Dump summary information of each file
        // if (vm.count("version"))
        //     call.reader.connect([](auto&& log) { std::cout 
        //             << "PLog Version " 
        //             << std::to_string(log.get_header().version_major) << "." 
        //             << std::to_string(log.get_header().version_minor) << "." 
        //             << std::to_string(log.get_header().version_subminor) << ", "
        //             << "built " << log.get_header().build_date << std::endl; });

        // if (vm.count("modules"))
        //     call.reader.connect([](auto&& log) { std::cout << log.get_header().modules << std::endl; });

        // if (vm.count("types"))
        //     call.reader.connect([](auto&& log) { std::cout << log.get_header().type_supports << std::endl; });

        // Dump one line headers from each record to stdout.
        if (vm.count("headers"))
            call.record.connect([](auto record) { 
                    std::cout << record.index << ": " << record.size << " bytes, type " 
                    << msg_type.at(record.msg_header.type)
                    << std::endl; 
                    });

        // Dump entire records to stdout
        if (vm.count("dump"))
            call.record.connect([](const plog::log_record& record) { 
                    std::cout << record << std::endl; 
                    std::istringstream iss(record.blob);
                    parse_description("ps_byte_array_msg", iss);
                   });


        // Count records
        size_t count = 0;
        if (vm.count("count")) {
            call.record.connect([&count](auto&&) { count += 1; });
            call.cleanup.connect([&count](auto&&) { 
                    std::cout << count << " records" << std::endl; 
                    count = 0;
                    });
        }

    }


};


boost::shared_ptr<transcode::plugin> create_plugin() {
    return boost::make_shared<struct plugin>();
}

}}} // namespace polysync::transcode::query

BOOST_DLL_ALIAS(polysync::transcode::query::create_plugin, query_plugin)


