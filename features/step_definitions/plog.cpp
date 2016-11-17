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
    std::string blob; // Binary sequence of bits
    polysync::tree tree;
};

using cucumber::ScenarioScope;

GIVEN("^a descriptor catalog$") {
    descriptor::catalog.emplace("ps_byte_array_msg", ps_byte_array_msg);
    descriptor::catalog.emplace("ibeo.header", ibeo_header);
}

GIVEN("^the hex blob (.+)$") {
    REGEX_PARAM(std::string, spaced_hex);
    ScenarioScope<DecodeContext> context;

    // Get rid of spaces used in Gherkin for readability
    std::string hex = spaced_hex;
    hex.erase(std::remove(hex.begin(), hex.end(), ' '), hex.end());

    // Decode the hex string into a binary blob
    mp::export_bits(mp::cpp_int("0x" + hex), std::back_inserter(context->blob), 8);
}

WHEN("^decoding the type (.+)$") {
    REGEX_PARAM(std::string, name);
    if (!descriptor::catalog.count(name))
        throw polysync::error("descriptor::catalog missing " + name);
    ScenarioScope<DecodeContext> context;
    const descriptor::type& desc = descriptor::catalog.at(name);
    std::stringstream stream(context->blob);
    plog::decoder decode(stream);
    context->tree = *decode(desc).target<polysync::tree>();
    if (stream.tellg() < decode.endpos)
        context->tree->emplace_back(decode.decode_desc("raw"));
}

THEN("^the result should be a (.+)$") {
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
