#pragma once

#include <stdexcept>
#include <boost/exception/exception.hpp>
#include <boost/exception/info.hpp>

namespace polysync { 

struct error : virtual std::runtime_error, virtual boost::exception {
    using std::runtime_error::runtime_error;
};

namespace exception {

using message = boost::error_info<struct tag_type, std::string>;
using type = boost::error_info<struct tag_type, std::string>;

} // namespace exception

} // namespace polysync
