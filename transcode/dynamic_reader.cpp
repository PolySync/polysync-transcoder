#include <polysync/transcode/dynamic_reader.hpp>
#include <polysync/transcode/io.hpp>

#include <algorithm>

std::string print(const std::vector<std::string>& p) { 
    return std::accumulate(p.begin(), p.end(), std::string(), [](auto s, auto f) { return s + "1"; }); 
}

namespace polysync { namespace plog { 

    using logging::severity;

    static std::vector<std::function<std::string (dynamic_reader&, tree&)>> type_detect = {

        [](dynamic_reader& read, tree& parent) -> std::string {

            for (const detector_type& detector: detector_list) {
                if (detector.parent != parent.name) {
                    BOOST_LOG_SEV(read.log, severity::debug2) << parent.name << ": parent is not " << detector.parent << " (no match)";
                    continue;
                }
                std::vector<std::string> mismatch;
                for (auto field: detector.match) {
                    auto it = parent.find(field.first);
                    if (it != parent.end() && (it->second == field.second)) {
                    } else
                        mismatch.emplace_back(field.first);
                }
                if (mismatch.empty()) {
                    BOOST_LOG_SEV(read.log, severity::debug1) << "parsing sequel \"" << detector.parent << "\" --> \"" << detector.child << "\"";
                    return detector.child;
                }
                BOOST_LOG_SEV(read.log, severity::debug2) << detector.child << ": mismatched" 
                    << std::accumulate(mismatch.begin(), mismatch.end(), std::string(), 
                            [&](auto str, auto field) { 
                            return str + " { " + field + ": " + lex(parent.at(field)) + " != " 
                            + lex(detector.match.at(field)) + " }"; });
            }
            return std::string();
        }
    };


    void parse_buffer(dynamic_reader& read, std::shared_ptr<tree> parent, std::vector<node>& result) {
        std::streampos rem = read.endpos - read.stream.tellg();
        BOOST_LOG_SEV(read.log, severity::debug2) << "parsing " << (size_t)rem << " byte buffer from " << parent->name;
        for (auto check: type_detect) {
            std::string name = check(read, *parent);
             if (!name.empty()) {
                 BOOST_LOG_SEV(read.log, severity::debug1) << "recursing nested type \"" << parent->name << "\" --> \"" << name << "\"";
                 node n = read(name, parent);
                 result.emplace_back(n);
                 std::streampos rem = read.endpos - read.stream.tellg();
                 if (rem > 0) {
                      std::shared_ptr<tree> tr = *n.target<std::shared_ptr<tree>>();
                      parse_buffer(read, tr, result);
                 };
                 return;
             } 
        }
        BOOST_LOG_SEV(read.log, severity::debug1) << "type not detected, returning raw sequence";
        plog::sequence<std::uint32_t, std::uint8_t> raw;
        raw.resize(rem);
        read.stream.read((char *)raw.data(), rem);
        result.emplace_back(raw);
        result.back().name = "raw";
    }

    using parser = std::function<node (reader&, std::shared_ptr<tree>)>;
    static std::map<std::string, parser> parse_map = {
        { "float", [](plog::reader& r, std::shared_ptr<tree>) { return r.read<float>(); } },
        { ">float32", [](plog::reader& r, std::shared_ptr<tree>) 
            {
                std::uint32_t swap = r.read<endian::big_uint32_t>().value();
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
        { ">uint16", [](plog::reader& r, std::shared_ptr<tree>){ return r.read<endian::big_uint16_t>(); } },
        { ">uint32", [](plog::reader& r, std::shared_ptr<tree>){ return r.read<endian::big_uint32_t>(); } },
        { ">uint64", [](plog::reader& r, std::shared_ptr<tree>){ return r.read<endian::big_uint64_t>(); } },
        { ">int16", [](plog::reader& r, std::shared_ptr<tree>){ return r.read<endian::big_int16_t>(); } },
        { ">int32", [](plog::reader& r, std::shared_ptr<tree>){ return r.read<endian::big_int32_t>(); } },
        { ">int64", [](plog::reader& r, std::shared_ptr<tree>){ return r.read<endian::big_int64_t>(); } },
        { "ps_guid", [](plog::reader& r, std::shared_ptr<tree>){ return r.read<plog::guid>(); } },
        { "ps_timestamp", [](plog::reader& r, std::shared_ptr<tree>) { return r.read<plog::timestamp>(); } },
        { "ps_msg_header", [](plog::reader& r, std::shared_ptr<tree>) { return r.read<plog::msg_header>(); } },
        { "log_record", [](plog::reader&, std::shared_ptr<tree>) { return (std::uint32_t)0; } },
        { "log_header", [](plog::reader&, std::shared_ptr<tree>) { return (std::uint32_t)0; } },
        { ">NTP64", [](plog::reader& r, std::shared_ptr<tree>){ return r.read<endian::big_uint64_t>(); } },
        { "sequence<octet>", [](plog::reader& r, std::shared_ptr<tree> parent) -> node
            { 
                auto seq = r.read<plog::sequence<std::uint32_t, std::uint8_t>>();
                BOOST_LOG_SEV(r.log, severity::debug2) << "parsing sequence with " << seq.size() << " bytes";
                std::istringstream sub(seq);
                dynamic_reader read(sub);
                std::vector<node> result;
                parse_buffer(read, parent, result); 
                if (result.size() == 1)
                    return result.front();

                std::shared_ptr<tree> tr = std::make_shared<tree>();
                for (auto n: result)
                    tr->emplace(n.name, n);
                return tr;
            } },
    };

    node dynamic_reader::operator()() {
        std::shared_ptr<tree> result = std::make_shared<tree>();

    // There is no feasible way to detect and enforce that a blob starts with a
    // msg_header.  Hence, we must just assume that every message is well
    // formed and starts with a msg_header.  In that case, might as well do a
    // static parse on msg_header and dynamic parse the rest.
    std::stringstream ss;
    ss << "msg_header";
    std::string name = ss.str();
    order += 1;
    msg_header msg = read<msg_header>();
    if (!type_support_map.count(msg.type))
        throw std::runtime_error("no type_support for " + std::to_string(msg.type));
    std::string type = type_support_map.at(msg.type);
    if (!description_map.count(type))
        throw std::runtime_error("no description for type \"" + type + "\"");
    const type_descriptor& desc = description_map.at(type);
    result->emplace(name, msg);

    std::shared_ptr<tree> tr = operator()(type, desc);
    for (auto n: *tr) // **(tr.target<std::shared_ptr<tree>>())) 
        result->emplace(n);
    node n = result;
    n.name = type; 
    return std::move(n);
}

std::shared_ptr<tree> dynamic_reader::operator()(const std::string& name, const type_descriptor& desc) {
    std::shared_ptr<tree> result = std::make_shared<tree>();
    result->name = name;
    std::for_each(desc.begin(), desc.end(), [this, &result](auto field) {
            if (field.name == "skip") {
                stream.seekg(std::stoul(field.type), std::ios_base::cur);
                return;
            }

            node a = operator()(field.type, result);
            // Attempt to keep the fields in the same order as the description.
            std::stringstream ss;
            order += 1;
            std::string name = ss.str();

            // fields that have been parsed recursively start out as
            // sequence<octet>, but a better, more specific name is discovered
            // during the parse.  If this happened, use the better name.
            // Otherwise, use the original descriptor's name.
            name = name + (a.name.empty() ? field.name : a.name);
            result->emplace(name, a);
    });
    return std::move(result);
}

node dynamic_reader::operator()(const std::string& type, std::shared_ptr<tree> parent) {

    auto parse = parse_map.find(type);
    if (parse != parse_map.end()) {
        node result = parse->second(*this, parent);
        return std::move(result);
    }

    auto desc = description_map.find(type);
    if (desc != description_map.end()) {
        node result = operator()(type, desc->second);
        result.name = type;
        return result;
    }

    throw std::runtime_error("dynamic_reader: no reader for type " + type);
}


}}  // polysync::plog
