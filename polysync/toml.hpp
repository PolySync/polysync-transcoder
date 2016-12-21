#pragma once

namespace polysync { namespace toml {

namespace fs = boost::filesystem;
namespace po = boost::program_options;

extern po::options_description load( const std::vector<fs::path>& );

}} // namespace polysync::toml


