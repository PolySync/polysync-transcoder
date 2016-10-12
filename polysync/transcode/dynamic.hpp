#pragma once

#include <polysync/transcode/core.hpp>
#include <eggs/variant.hpp>
#include <boost/endian/buffers.hpp>

namespace polysync { namespace plog {

namespace endian = boost::endian;

using atom = eggs::variant<log_header, sequence<std::uint32_t, std::uint8_t>, 
      std::int8_t, std::int16_t, int32_t, int64_t,
      std::uint8_t, std::uint16_t, uint32_t, uint64_t,
      endian::big_uint16_buf_t, endian::big_uint32_buf_t, endian::big_uint64_buf_t,
      endian::big_int16_buf_t, endian::big_int32_buf_t, endian::big_int64_buf_t>;

}} // namespace polysync::plog
