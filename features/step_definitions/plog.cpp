#include <mettle/matchers.hpp>
#include <polysync/plog/decoder.hpp>
#include <cucumber-cpp/autodetect.hpp>

using namespace mettle;

namespace descriptor = polysync::descriptor;
namespace plog = polysync::plog;
namespace exception = polysync::exception;
namespace mp = boost::multiprecision;

descriptor::type ps_byte_array_msg { "ps_byte_array_msg", { 
        { "dest_guid", typeid(plog::guid) },
        { "data_type", typeid(uint32_t) },
        { "payload", typeid(uint32_t) } }
};

descriptor::type ibeo_header { "ibeo.header", {
    { "magic", typeid(std::uint32_t) },
    { "prev_size", typeid(std::uint32_t) },
    { "size", typeid(std::uint8_t) },
    { "skip", polysync::descriptor::skip { 4 } },
    { "device_id", typeid(std::uint8_t) },
    { "data_type", typeid(std::uint16_t) },
    { "time", typeid(std::uint64_t) } }
};

polysync::descriptor::type scanner_info { "scanner_info", {
    { "device_id", typeid(std::uint8_t) },
    { "scanner_type", typeid(std::uint8_t) } }
};

polysync::descriptor::type nested_scanner { "nested_scanner", {
    { "start_time", typeid(std::uint16_t) },
    { "scanner_info_list", polysync::descriptor::nested{"scanner_info"} } }
};
 
std::map<std::string, std::function<polysync::variant (std::string)>> factory {
    { "int8", [](auto value) { return static_cast<std::int8_t>(std::stoul(value)); } },
    { "int16", [](auto value) { return static_cast<std::int16_t>(std::stoul(value)); } },
    { "int32", [](auto value) { return static_cast<std::int32_t>(std::stoul(value)); } },
    { "int64", [](auto value) { return static_cast<std::int64_t>(std::stoull(value)); } },
    { "uint8", [](auto value) { return static_cast<std::uint8_t>(std::stoul(value)); } },
    { "uint16", [](auto value) { return static_cast<std::uint16_t>(std::stoul(value)); } },
    { "uint32", [](auto value) { return static_cast<std::uint32_t>(std::stoul(value)); } },
    { "uint64", [](auto value) { return static_cast<std::uint64_t>(std::stoull(value)); } },
    { "plog::guid", [](auto value) { return static_cast<plog::guid>(std::stoull(value)); } },
};


struct DecodeContext {
    std::stringstream blob;
    polysync::tree tree;
};

using cucumber::ScenarioScope;

static std::map<std::string, const descriptor::type&> test_catalog {
    { "ps_byte_array_msg", ps_byte_array_msg },
    { "ibeo.header", ibeo_header },
    { "nested_scanner", nested_scanner },
    { "scanner_info", scanner_info },
};

GIVEN("an empty descriptor catalog") {
    descriptor::catalog.clear();
}

GIVEN("^the hex blob(.+)?$") {
    REGEX_PARAM(std::string, smallblob);
    std::string hex = smallblob; // need a non-const string

    // Using the docstring for the hex blob is also allowed
    try {
        REGEX_PARAM(std::string, docstring);
        hex += docstring;
    } catch (std::invalid_argument) {
        // No problem, the docstring is optional
    }

    // Pull out hex numbers from whitespace, and blow away comments marked with '#'
    std::regex strip_regex("\\s*([0-9,A-F]+)\\s*(#.*\\n)?");
    auto hex_begin = std::sregex_iterator(hex.begin(), hex.end(), strip_regex);
    auto hex_end = std::sregex_iterator();
    hex = std::accumulate(hex_begin, hex_end, std::string(), 
            [](const std::string& h, std::smatch match) { return h + match[1].str(); });

    ScenarioScope<DecodeContext> context;

    // Decode the hex string into a binary blob
    mp::export_bits(mp::cpp_int("0x" + hex), 
            std::ostreambuf_iterator<char>(context->blob), 8);
}

GIVEN("^the descriptor catalog contains (.+)$") {
    REGEX_PARAM(std::string, name);
    if (!test_catalog.count(name))
        throw polysync::error("test catalog missing " + name);
    descriptor::catalog.emplace(name, test_catalog.at(name));
}

WHEN("^decoding the type ([^\\s]+)$") {
    REGEX_PARAM(std::string, name);
    if (!descriptor::catalog.count(name))
        throw polysync::error("descriptor catalog missing " + name);
    const descriptor::type& desc = descriptor::catalog.at(name);
    ScenarioScope<DecodeContext> context;
    plog::decoder decode(context->blob);
    context->tree = *decode(desc).target<polysync::tree>();
    if (context->blob.tellg() < decode.endpos)
        context->tree->emplace_back(decode.decode_desc("raw"));
}

THEN("^the result should be an? (.+)$") {
    REGEX_PARAM(std::string, name);
    TABLE_PARAM(fieldsParam);
    ScenarioScope<DecodeContext> context;

    auto fields = fieldsParam.hashes();
    polysync::tree truth = polysync::tree::create(name);
    for (auto it = fields.begin(); it != fields.end(); ++it) {
        std::string name(it->at("name"));
        std::string type(it->at("type"));
        std::string value(it->at("value"));
        truth->emplace_back(polysync::node (name, factory.at(type)(value) ));
    }
    expect(context->tree, equal_to(truth));

}

THEN("^decoding the type (.+) should throw \"(.+)\"$") {
    REGEX_PARAM(std::string, name);
    REGEX_PARAM(std::string, message);

    ScenarioScope<DecodeContext> context;

    expect([&]() { 
            plog::decoder(context->blob).decode_desc(name); 
            }, thrown<polysync::error>(message));
}
