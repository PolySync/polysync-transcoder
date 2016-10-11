#include <polysync/transcode/core.hpp>

namespace polysync { namespace plog {
    
// These descriptions will become the fallback descriptions for legacy plogs,
// used when the plog lacks self descriptions.  New files should have these
// embedded, rather than use these.
std::map<std::string, std::vector<plog::field_descriptor>> description_map
{
    { "ps_byte_array_msg", { { "header", "log_record" },
                             { "guid", "ps_guid" },
                             { "data_type", "uint32" },
                             { "bytes", "sequence<octet>" } 
                           } },
    { "ibeo.header", { { "magic", "uint32" },
                         { "prev_size", "uint32" },
                         { "size", "uint32" },
                         { "reserved2", "uint8" },
                         { "device_id", "uint8" },
                         { "data_type", "uint16" },
                         { "time", "NTP64" },
                         { "data", "sequence<octet>" }
                     } },
    { "ibeo.vehicle_state", {
                                { "reserved", "uint32" },
                                { "timestamp", "ps_timestamp" },
                                { "distance_x", "int32" },
                                { "distance_y", "int32" }
                            } },
                            
};

}} // namespace polysync::plog
