#pragma once

#include <stdexcept>
#include <boost/exception/exception.hpp>
#include <boost/exception/all.hpp>

namespace polysync { 

// Define a custom exception, and use boost::exception to incrementally add context as the stack unwinds during an exception.
struct error : virtual std::runtime_error, virtual boost::exception {
    using std::runtime_error::runtime_error;
};

// Define a set of return values for shell programs that check.
enum status : int {
    ok = 0, // No error
    bad_input = -2, // Error configuring the input dataset
    no_plugin = -3, // Error configuring the output plugin 
    description_error = -4, // Problem with a TOML file
};

namespace exception {

using type = boost::error_info<struct type_type, std::string>;
using detector = boost::error_info<struct detector_type, std::string>;
using field = boost::error_info<struct field_type, std::string>;
using plugin = boost::error_info<struct plugin_type, std::string>;
using status = boost::error_info<struct status_type, polysync::status>;
using path = boost::error_info<struct path_type, std::string>;
using module = boost::error_info<struct module_type, std::string>;

} // namespace exception

inline error operator<<(error e, status s) {
    e << exception::status(s);
    return e;
}


} // namespace polysync
