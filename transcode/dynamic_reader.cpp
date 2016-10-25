#include <polysync/transcode/dynamic_reader.hpp>
#include <polysync/transcode/io.hpp>

#include <algorithm>

std::string print(const std::vector<std::string>& p) { 
    return std::accumulate(p.begin(), p.end(), std::string(), [](auto s, auto f) { return s + "1"; }); 
}

namespace polysync { namespace plog { 

using logging::severity;

node parse_buffer(dynamic_reader& read, const std::vector<node>& series) {
    if (series.empty())
        throw std::runtime_error("series has no parent");
    const node& parent = series.back();
    if (parent.name.empty())
        throw std::runtime_error("node has no name");

    std::shared_ptr<plog::tree> tree;
    tree = *parent.target<std::shared_ptr<plog::tree>>();
    if (tree->empty())
        throw std::runtime_error("parent tree is empty");

    std::streampos rem = read.endpos - read.stream.tellg();
    BOOST_LOG_SEV(read.log, severity::debug2) 
        << "decoding " << (size_t)rem << " byte payload from \"" << parent.name << "\"";

    // Iterate each detector in the catalog and check for a match.  Store the resulting type name in tpname.
    std::string tpname;
    for (const detector::type& det: detector::catalog) {

        // Parent is not even the right type, so short circuit and fail this test early.
        if (det.parent != parent.name) {
            BOOST_LOG_SEV(read.log, severity::debug2) << det.child << " not matched: parent \"" 
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
                BOOST_LOG_SEV(read.log, severity::debug2) << det.child << " not matched: parent \"" << det.parent << "\" missing field \"" << field.first << "\"";
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
        BOOST_LOG_SEV(read.log, severity::debug2) << det.child << ": mismatched" 
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
        BOOST_LOG_SEV(read.log, severity::debug1) << "type not detected, returning raw sequence";
        plog::sequence<std::uint32_t, std::uint8_t> raw;
        raw.resize(rem);
        read.stream.read((char *)raw.data(), rem);
        return node(raw, "raw");
    }

    BOOST_LOG_SEV(read.log, severity::debug1) << tpname << " matched from parent \"" << parent.name << "\"";

    // We have exactly one detector match. This is the best case. Recurse and decode sequel.
    // series.emplace_back(read.read(tpname, series));
    return read.read(tpname, series);
}

using parser = std::function<node (reader&, const std::vector<node>&)>;
static std::map<std::string, parser> parse_map = {
    { "float", [](reader& r, const std::vector<node>&) { return r.read<float>(); } },
    { ">float32", [](reader& r, const std::vector<node>&) 
        {
            std::uint32_t swap = r.read<endian::big_uint32_t>().value();
            return *(new ((void *)&swap) float);
        } },
    { "double", [](reader& r, const std::vector<node>&) { return r.read<double>(); } },
    { "float64", [](reader& r, const std::vector<node>&) { return r.read<double>(); } },
    { "uint8", [](reader& r, const std::vector<node>&) { return r.read<std::uint8_t>(); } },
    { "uint16", [](reader& r, const std::vector<node>&) { return r.read<std::uint16_t>(); } },
    { "uint32", [](reader& r, const std::vector<node>&) { return r.read<std::uint32_t>(); } },
    { "uint64", [](reader& r, const std::vector<node>&) { return r.read<std::uint64_t>(); } },
    { "int8", [](reader& r, const std::vector<node>&) { return r.read<std::int8_t>(); } },
    { "int16", [](reader& r, const std::vector<node>&) { return r.read<std::int16_t>(); } },
    { "int32", [](reader& r, const std::vector<node>&) { return r.read<std::int32_t>(); } },
    { "int64", [](reader& r, const std::vector<node>&) { return r.read<std::int64_t>(); } },
    { ">uint16", [](reader& r, const std::vector<node>&){ return r.read<endian::big_uint16_t>(); } },
    { ">uint32", [](reader& r, const std::vector<node>&){ return r.read<endian::big_uint32_t>(); } },
    { ">uint64", [](reader& r, const std::vector<node>&){ return r.read<endian::big_uint64_t>(); } },
    { ">int16", [](reader& r, const std::vector<node>&){ return r.read<endian::big_int16_t>(); } },
    { ">int32", [](reader& r, const std::vector<node>&){ return r.read<endian::big_int32_t>(); } },
    { ">int64", [](reader& r, const std::vector<node>&){ return r.read<endian::big_int64_t>(); } },
    { "ps_guid", [](reader& r, const std::vector<node>&){ return r.read<plog::guid>(); } },
    { "ps_timestamp", [](reader& r, const std::vector<node>&) { return r.read<plog::timestamp>(); } },
    { ">NTP64", [](reader& r, const std::vector<node>&){ return r.read<endian::big_uint64_t>(); } },
};

node dynamic_reader::read() {

    // There is no feasible way to detect and enforce that a blob starts with a
    // msg_header.  Hence, we must just assume that every message is well
    // formed and starts with a msg_header.  In that case, might as well do a
    // static parse on msg_header and dynamic parse the rest.
    msg_header msg = read<msg_header>();
    std::vector<node> series = { { node::from(msg), "msg_header" } };
    while (endpos - stream.tellg() > 0) {
        node n = parse_buffer(*this, series);
        series.emplace_back(n);
    }
    std::shared_ptr<plog::tree> tree = std::make_shared<plog::tree>();
    std::copy(series.begin(), series.end(), std::back_inserter(*tree));
    return node(tree, "log_record");
}

node dynamic_reader::read(const std::string& type, const std::vector<node>& series) {

    auto parse = parse_map.find(type);
    if (parse != parse_map.end()) 
        return parse->second(*this, series);

    if (!descriptor::catalog.count(type))
        throw std::runtime_error("dynamic_reader: no reader for type " + type);

    const descriptor::type& desc = descriptor::catalog.at(type);
    std::shared_ptr<plog::tree> child = std::make_shared<plog::tree>();
    BOOST_LOG_SEV(log, severity::debug2) << "decoding \"" << type << "\"";
    std::for_each(desc.begin(), desc.end(), [&](auto field) {

            // Burn off unused or reserved space using the "skip" keyword.
            if (field.name == "skip") {
                stream.seekg(std::stoul(field.type), std::ios_base::cur);
                return;
            }

            node a = read(field.type, series);
            BOOST_LOG_SEV(log, severity::debug2) << field.name << " = " << a << " (" << field.type << ")";

            // fields that have been parsed recursively start out as
            // sequence<octet>, but a better, more specific name is discovered
            // during the parse.  If this happened, use the better name.
            // Otherwise, use the original descriptor's name.
            std::string fname = a.name.empty() ? field.name : a.name;
            child->emplace_back(a, fname);
        });

    return node(child, type);
}


}}  // polysync::plog
