#include <polysync/transcode/dynamic_reader.hpp>

namespace polysync { namespace plog {
    
// These descriptions will become the fallback descriptions for legacy plogs,
// used when the plog lacks self descriptions.  New files should have these
// embedded, rather than use these.
// std::map<std::string, detector_type> detector_map {
std::vector<detector_type> detector_list;
// {
//     { "ps_byte_array_msg", { { "data_type", (std::uint32_t)160 } }, "ibeo.header" },
//     { "ibeo.header", { 
//                          { "magic", (std::uint32_t)0xAFFEC0C2 },
//                          { "data_type", (std::uint16_t)0x2807 } 
//                      }, "ibeo.vehicle_state" },
// };

std::map<std::string, type_descriptor> description_map;
// {
//     { "ps_byte_array_msg", { 
//                              { "dest_guid", "ps_guid" },
//                              { "data_type", "uint32" },
//                              { "payload", "sequence<octet>" } 
//                            } },
//     { "ibeo.header", { 
//                        { "magic", ">uint32" },
//                        { "prev_size", ">uint32" },
//                        { "size", ">uint32" },
//                        { "skip", "uint8" },
//                        { "device_id", "uint8" },
//                        { "data_type", ">uint16" },
//                        { "time", ">NTP64" },
//                      } },
//     { "ibeo.vehicle_state", 
//         {
//             { "skip", "uint32" },
//             { "timestamp", ">NTP64" },
//             { "distance_x", ">int32" },
//             { "distance_y", ">int32" },
//             { "course_angle", ">float32" },
//             { "longitudinal_velocity", ">float32" },
//             { "yaw_rate", ">float32" },
//             { "steering_wheel_angle", ">float32" },
//             { "skip", "uint32" },
//             { "front_wheel_angle", ">float32" },
//             { "skip", "uint16" },
//             { "vehicle_width", ">float32" },
//             { "skip", "uint32" },
//             { "distance_axle_front_to_front", ">float32" },
//             { "distance_axle_front_to_rear", ">float32" },
//             { "distance_axle_rear_to_rear", ">float32" },
//             { "skip", "uint32" },
//             { "steer_ratio_poly_1", ">float32" },
//             { "steer_ratio_poly_2", ">float32" },
//             { "steer_ratio_poly_3", ">float32" },
//             { "longitudinal_acceleration", ">float32" },
//         } },
// 
// };

}} // namespace polysync::plog
