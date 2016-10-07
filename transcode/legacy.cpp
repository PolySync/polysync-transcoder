#include <polysync/transcode/core.hpp>

namespace polysync { namespace plog {
    
// std::vector<plog::field_descriptor> log_header {
//     { "version_major", "uint8" },
//     { "version_minor", "uint8" },
//     { "version_subminor", "uint16" },
//     { "build_date", "uint32" },
//     { "node_guid", "uint64" },
//     { "modules", "sequence<uint32>" },
//     { "type_supports", "sequence<uint32>" }
// };


// These descriptions will become the fallback descriptions for legacy plogs,
// used when the plog lacks self descriptions.  New files should have these
// embedded, rather than use these.
std::map<std::string, std::vector<plog::field_descriptor>> description_map
{
    { "ps_byte_array_msg", { { "header", "log_record" },
                             { "guid", "ps_guid" },
                             { "data_type", "uint32" },
                             { "bytes", "sequence<octet>" } } }
};

}} // namespace polysync::plog
