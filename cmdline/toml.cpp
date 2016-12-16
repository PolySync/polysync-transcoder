#include <regex>

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>

#include <deps/cpptoml.h>

#include <polysync/exception.hpp>
#include <polysync/logging.hpp>
#include <polysync/descriptor.hpp>
#include <polysync/detector.hpp>

namespace po = boost::program_options;
namespace fs = boost::filesystem;

namespace polysync { namespace toml {

using logging::severity;
using logging::logger;

static logger log( "toml" );

po::options_description load( const std::vector<fs::path>& plugpath ) {

    for ( fs::path descdir: plugpath ) {

        descdir = descdir / "share";
        if ( !fs::exists( descdir ) ) {
            BOOST_LOG_SEV( log, severity::debug1 )
                << "skipping description path " << descdir
                << " because it does not exist";
            continue;
        }

        BOOST_LOG_SEV( log, severity::debug1 ) << "searching " << descdir << " for type descriptions";
        static std::regex is_description( R"((.+)\.toml)" );
        for ( fs::directory_entry& tofl: fs::directory_iterator( descdir ) ) {
            std::cmatch match;
            std::regex_match( tofl.path().string().c_str(), match, is_description );
            if (match.size()) {
                BOOST_LOG_SEV( log, severity::debug1 ) << "loading descriptions from " << tofl;
                try {
                    std::shared_ptr<cpptoml::table> descfile = cpptoml::parse_file( tofl.path().string() );

                    // Parse the file in two passes, so the detectors have
                    // access to the descriptor's types
                    for ( const auto& type: *descfile ) {
                        if ( type.second->is_table() )
                            descriptor::load(
                                    type.first, type.second->as_table(), descriptor::catalog );
                        else if ( type.second->is_value() ) {
                            auto val = type.second->as<std::string>();
                            if ( !descriptor::namemap.count(val->get()) )
                                throw error( "unknown type alias" )
                                    << exception::type(type.first);
                            std::type_index idx = descriptor::namemap.at(val->get());
                            descriptor::namemap.emplace( type.first, idx );
                            BOOST_LOG_SEV( log, severity::debug2 )
                                << "loaded type alias " << type.first << " = " << val->get();
                        } else
                            BOOST_LOG_SEV( log, severity::warn ) << "unused description: " << type.first;
                    }
                    for ( const auto& type: *descfile )
                        if ( type.second->is_table() )
                            detector::load( type.first, type.second->as_table(), detector::catalog );
                } catch ( error& e ) {
                    e << status::description_error;
                    e << exception::path( tofl.path().string() );
                    throw;
                } catch ( cpptoml::parse_exception& e ) {
                    throw polysync::error( e.what() )
                        << status::description_error
                        << exception::path( tofl.path().string() );
                }
            }
        }
    }

    po::options_description opts("Type Description Options");
    return opts;
}

}} // namespace polysync::toml

