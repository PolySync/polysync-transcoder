#include <polysync/transcode/dynamic_reader.hpp>
#include <polysync/transcode/io.hpp>

namespace polysync { namespace plog { 

using logging::severity;

static std::vector<std::function<std::string (dynamic_reader&, tree&)>> type_detect = {

    [](dynamic_reader& read, tree& parent) -> std::string {

        std::map<size_t, std::string> sensor_support = {
            { 160, "ibeo.vehicle_state" }
        };

        auto dti = parent.find("data_type");
        if (dti != parent.end()) {
            std::uint32_t sensor = *(dti->second.target<std::uint32_t>());
            auto si = sensor_support.find(sensor);
            if (si != sensor_support.end())
                return si->second;
        }
        return std::string();
    },
};

using parser = std::function<node (reader&, std::shared_ptr<tree>)>;
static std::map<std::string, parser> parse_map = {
    { "float", [](plog::reader& r, std::shared_ptr<tree>) { return r.read<float>(); } },
    { ">float32", [](plog::reader& r, std::shared_ptr<tree>) 
        {
            std::uint32_t swap = r.read<endian::big_uint32_buf_t>().value();
            return *(new ((void *)&swap) float);
        } },
    { "double", [](plog::reader& r, std::shared_ptr<tree>) { return r.read<double>(); } },
    { "float64", [](plog::reader& r, std::shared_ptr<tree>) { return r.read<double>(); } },
    { "uint8", [](plog::reader& r, std::shared_ptr<tree>) { return r.read<std::uint8_t>(); } },
    { "uint16", [](plog::reader& r, std::shared_ptr<tree>) { return r.read<std::uint16_t>(); } },
    { "uint32", [](plog::reader& r, std::shared_ptr<tree>) { return r.read<std::uint32_t>(); } },
    { "uint64", [](plog::reader& r, std::shared_ptr<tree>) { return r.read<std::uint64_t>(); } },
    { "int8", [](plog::reader& r, std::shared_ptr<tree>) { return r.read<std::int8_t>(); } },
    { "int16", [](plog::reader& r, std::shared_ptr<tree>) { return r.read<std::int16_t>(); } },
    { "int32", [](plog::reader& r, std::shared_ptr<tree>) { return r.read<std::int32_t>(); } },
    { "int64", [](plog::reader& r, std::shared_ptr<tree>) { return r.read<std::int64_t>(); } },
    { ">uint16", [](plog::reader& r, std::shared_ptr<tree>){ return r.read<endian::big_uint16_buf_t>(); } },
    { ">uint32", [](plog::reader& r, std::shared_ptr<tree>){ return r.read<endian::big_uint32_buf_t>(); } },
    { ">uint64", [](plog::reader& r, std::shared_ptr<tree>){ return r.read<endian::big_uint64_buf_t>(); } },
    { ">int16", [](plog::reader& r, std::shared_ptr<tree>){ return r.read<endian::big_int16_buf_t>(); } },
    { ">int32", [](plog::reader& r, std::shared_ptr<tree>){ return r.read<endian::big_int32_buf_t>(); } },
    { ">int64", [](plog::reader& r, std::shared_ptr<tree>){ return r.read<endian::big_int64_buf_t>(); } },
    { "ps_guid", [](plog::reader& r, std::shared_ptr<tree>){ return r.read<plog::guid>(); } },
    { "ps_timestamp", [](plog::reader& r, std::shared_ptr<tree>) { return r.read<plog::timestamp>(); } },
    { "ps_msg_header", [](plog::reader& r, std::shared_ptr<tree>) { return r.read<plog::msg_header>(); } },
    { "log_record", [](plog::reader&, std::shared_ptr<tree>) { return (std::uint32_t)0; } },
    { "log_header", [](plog::reader&, std::shared_ptr<tree>) { return (std::uint32_t)0; } },
    { ">NTP64", [](plog::reader& r, std::shared_ptr<tree>){ return r.read<endian::big_uint64_buf_t>(); } },
    { "sequence<octet>", [](plog::reader& r, std::shared_ptr<tree> parent) -> node
        { 
            auto seq = r.read<plog::sequence<std::uint32_t, std::uint8_t>>();
            BOOST_LOG_SEV(r.log, severity::debug2) << "parsing sequence with " << seq.size() << " bytes";
            std::istringstream sub(seq);
            dynamic_reader read(sub);
            for (auto check: type_detect) {
                std::string name = check(read, *parent);
                if (!name.empty()) {
                    BOOST_LOG_SEV(r.log, severity::debug1) << "detected type \"" << name << "\"";
                    node n = read(name, parent);
                    // n.type = "type3"; // name;
                    return n;
                } 
            }
            BOOST_LOG_SEV(r.log, severity::info) << "type not detected, returning raw sequence";
            return seq;
        } },
};


node dynamic_reader::operator()(std::streamoff off, const std::string& type, std::shared_ptr<tree> parent) {
    stream.seekg(off);
    return operator()("type4", parent);
}

node dynamic_reader::operator()() {
    std::shared_ptr<tree> result = std::make_shared<tree>();

    // There is no feasible way to detect and enforce that a blob starts with a
    // msg_header.  Hence, we must just assume that every message is well
    // formed and starts with a msg_header.  In that case, might as well do a
    // static parse on msg_header and dynamic parse the rest.
    msg_header msg = read<msg_header>();
    std::string type = type_support_map.at(msg.type);
    const type_descriptor& desc = description_map.at(type);
    node tr = operator()(desc);
    result->emplace("msg_header", msg);
    for (auto n: **(tr.target<std::shared_ptr<tree>>())) 
        result->emplace(n);
    node n = result;
    n.name = type; 
    return std::move(n);
}

node dynamic_reader::operator()(const std::string& type, std::shared_ptr<tree> parent) {

    auto parse = parse_map.find(type);
    if (parse != parse_map.end()) {
        node result = parse->second(*this, parent);
        BOOST_LOG_SEV(log, severity::debug2) << "parsed dynamic " << type << " = " << std::hex << result << std::dec;
        // result.name = "type1"; // type;
        return std::move(result);
    }

    auto desc = description_map.find(type);
    if (desc != description_map.end()) {
        node result = operator()(desc->second);
        BOOST_LOG_SEV(log, severity::debug2) << "parsed from description " << result;
        result.name = type;
        return result;
    }

    throw std::runtime_error("dynamic_reader: no reader for type " + type);
}

node dynamic_reader::operator()(const type_descriptor& desc) {
    BOOST_LOG_SEV(log, severity::debug1) << "parsing " << desc;
    std::shared_ptr<tree> result = std::make_shared<tree>();
    std::for_each(desc.begin(), desc.end(), [this, &result](auto field) {
            node a = operator()(field.type, result);
            // fields that have been parsed recursively start out as
            // sequence<octet>, but a better, more specific name is discovered
            // during the parse.  If this happened, use the better name.
            // Otherwise, use the original descriptor's name.
            if (a.name.empty())
                result->emplace(field.name, a);
            else
                result->emplace(a.name, a);
    });
    return std::move(result);
}

}}  // polysync::plog
