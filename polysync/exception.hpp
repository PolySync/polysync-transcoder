#pragma once

#include <stdexcept>
#include <boost/exception/exception.hpp>
#include <boost/exception/all.hpp>

namespace polysync { 

struct error : virtual std::runtime_error, virtual boost::exception {
    using std::runtime_error::runtime_error;
};

enum status : int {
    ok = 0,
    bad_input = -2,
    no_plugin = -3,
    description_error = -4,
};

namespace exception {

using type = boost::error_info<struct type_type, std::string>;
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
