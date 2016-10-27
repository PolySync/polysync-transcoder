#include <polysync/plog/decoder.hpp>
#include <polysync/plog/io.hpp>

#include <algorithm>

std::string print(const std::vector<std::string>& p) { 
    return std::accumulate(p.begin(), p.end(), std::string(), [](auto s, auto f) { return s + "1"; }); 
}

namespace polysync { namespace plog { 

using logging::severity;

std::string decoder::detect(const node& parent) {
    plog::tree tree = *parent.target<plog::tree>();
    if (tree->empty())
        throw std::runtime_error("parent tree is empty");

    std::streampos rem = endpos - stream.tellg();
    BOOST_LOG_SEV(log, severity::debug2) 
        << "decoding " << (size_t)rem << " byte payload from \"" << parent.name << "\"";

    // Iterate each detector in the catalog and check for a match.  Store the resulting type name in tpname.
    std::string tpname;
    for (const detector::type& det: detector::catalog) {

        // Parent is not even the right type, so short circuit and fail this test early.
        if (det.parent != parent.name) {
            BOOST_LOG_SEV(log, severity::debug2) << det.child << " not matched: parent \"" 
                << parent.name << "\" != \"" << det.parent << "\"";
            continue;
        }

        // Iterate each field in the detector looking for mismatches.
        std::vector<std::string> mismatch;
        for (auto field: det.match) {
            auto it = std::find_if(tree->begin(), tree->end(), 
                    [field](const node& n) { 
                    return n.name == field.first; 
                    });
            if (it == tree->end()) {
                BOOST_LOG_SEV(log, severity::debug2) << det.child << " not matched: parent \"" << det.parent << "\" missing field \"" << field.first << "\"";
                break;
            }
            if (*it != field.second)
                mismatch.emplace_back(field.first);
        }
        
        // Too many matches. Catalog is not orthogonal and needs tweaking.
        if (mismatch.empty() && !tpname.empty())
            throw std::runtime_error("non-unique detectors: " + tpname + " and " + det.child);

        // Exactly one match. We have detected the sequel type.
        if (mismatch.empty()) {
            tpname = det.child;
            continue;
        }

        //  The detector failed, print a fancy message to help developer fix catalog.
        BOOST_LOG_SEV(log, severity::debug2) << det.child << ": mismatched" 
            << std::accumulate(mismatch.begin(), mismatch.end(), std::string(), 
                    [&](const std::string& str, auto field) { 
                        return str + " { " + field + ": " + 
                        // lex(*std::find_if(tree->begin(), tree->end(), [field](auto f){ return field == f.name; })) + 
                        // lex(tree->at(field)) + 
                        // (tree->at(field) == det.match.at(field) ? " == " : " != ")
                        lex(det.match.at(field)) + " }"; 
                    });
    }

    // Absent a detection, return raw bytes.
    if (tpname.empty()) {
        BOOST_LOG_SEV(log, severity::debug1) << "type not detected, returning raw sequence";
        return "raw";
    }

    BOOST_LOG_SEV(log, severity::debug1) << tpname << " matched from parent \"" << parent.name << "\"";

    return tpname;
}

// Kick off a decoder with an implict type "log_record" and starting type
// "msg_header".  Continue reading the stream until it ends.
node decoder::operator()(const log_record& record) {
    node result = node::from(record, "log_record");
    plog::tree tree = *result.target<plog::tree>();

    // There is no simple way to detect and enforce that a blob starts with a
    // msg_header.  Hence, we must just assume that every message is well
    // formed and starts with a msg_header.  In that case, might as well do a
    // static parse on msg_header and dynamic parse the rest.
    tree->emplace_back(node::from(decode<msg_header>(), "msg_header"));

    // Burn through the rest of the log record, decoding a sequence of types.
    while (endpos - stream.tellg() > 0) {
        std::string type = detect(tree->back());
        tree->emplace_back(decode_desc(type));
    }
    return result;
}

// Define a set of factory functions that know how to decode specific binary
// types.  They keys are strings from the "type" field of the TOML descriptions.
std::map<std::string, decoder::parser> decoder::parse_map = {
    { "float", [](decoder& r) { return r.decode<float>(); } },
    { ">float32", [](decoder& r) 
        {
            std::uint32_t swap = r.decode<endian::big_uint32_t>().value();
            return *(new ((void *)&swap) float);
        } },
    { "double", [](decoder& r) { return r.decode<double>(); } },
    { "float64", [](decoder& r) { return r.decode<double>(); } },
    { "uint8", [](decoder& r) { return r.decode<std::uint8_t>(); } },
    { "uint16", [](decoder& r) { return r.decode<std::uint16_t>(); } },
    { "uint32", [](decoder& r) { return r.decode<std::uint32_t>(); } },
    { "uint64", [](decoder& r) { return r.decode<std::uint64_t>(); } },
    { "int8", [](decoder& r) { return r.decode<std::int8_t>(); } },
    { "int16", [](decoder& r) { return r.decode<std::int16_t>(); } },
    { "int32", [](decoder& r) { return r.decode<std::int32_t>(); } },
    { "int64", [](decoder& r) { return r.decode<std::int64_t>(); } },
    { ">uint16", [](decoder& r){ return r.decode<endian::big_uint16_t>(); } },
    { ">uint32", [](decoder& r){ return r.decode<endian::big_uint32_t>(); } },
    { ">uint64", [](decoder& r){ return r.decode<endian::big_uint64_t>(); } },
    { ">int16", [](decoder& r){ return r.decode<endian::big_int16_t>(); } },
    { ">int32", [](decoder& r){ return r.decode<endian::big_int32_t>(); } },
    { ">int64", [](decoder& r){ return r.decode<endian::big_int64_t>(); } },
    { "ps_guid", [](decoder& r){ return r.decode<plog::guid>(); } },
    { "ps_timestamp", [](decoder& r) { return r.decode<plog::timestamp>(); } },
    { ">NTP64", [](decoder& r){ return r.decode<endian::big_uint64_t>(); } },
    { "raw", [](decoder& r) 
        { 
            plog::sequence<std::uint32_t, std::uint8_t> raw;
            std::streampos rem = r.endpos - r.stream.tellg();
            raw.resize(rem);
            r.stream.read((char *)raw.data(), rem);
            return node(raw, "raw", "raw");
        }},
};

// Read a field, described by looking up the type by string.  The type strings can
// be compound types described in the TOML description, primitive types known
// by parse_map, or strings generated by boost::hana from compile time structs.
node decoder::decode_desc(const std::string& type) {

    auto parse = parse_map.find(type);
    if (parse != parse_map.end()) 
        return parse->second(*this);

    if (!descriptor::catalog.count(type))
        throw std::runtime_error("no decoder for type " + type);

    BOOST_LOG_SEV(log, severity::debug2) << "decoding \"" << type << "\"";

    plog::tree child = std::make_shared<plog::tree::element_type>();
    const descriptor::type& desc = descriptor::catalog.at(type);
    std::for_each(desc.begin(), desc.end(), [&](auto field) {

            // Burn off unused or reserved space using the "skip" keyword.
            if (field.name == "skip") {
                stream.seekg(std::stoul(field.type), std::ios_base::cur);
                return;
            }

            node a = decode_desc(field.type);
            BOOST_LOG_SEV(log, severity::debug2) << field.name << " = " << a << " (" << field.type << ")";

            // fields that have been parsed recursively start out as
            // sequence<octet>, but a better, more specific name is discovered
            // during the parse.  If this happened, use the better name.
            // Otherwise, use the original descriptor's name.
            std::string fname = a.name.empty() ? field.name : a.name;
            child->emplace_back(a, fname, field.type);
        });

    return node(child, type, type);
}


}}  // polysync::plog
