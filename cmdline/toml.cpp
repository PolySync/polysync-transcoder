#include <regex>

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <boost/algorithm/string.hpp>

#include <cpptoml.h>

#include <polysync/exception.hpp>
#include <polysync/logging.hpp>
#include <polysync/descriptor.hpp>
#include <polysync/detector.hpp>
#include <polysync/print_vector.hpp>

namespace po = boost::program_options;
namespace fs = boost::filesystem;

namespace polysync { namespace toml {

using logging::severity;
using logging::logger;

static logger log( "typesupport" );

void analyzeFile( const fs::path& filename )
{
    std::shared_ptr<cpptoml::table> table =
        cpptoml::parse_file( filename.string() );

    // Parse the file in two passes, so the detectors have
    // access to the descriptor's types
    for ( const auto& type: *table )
    {
        if ( type.second->is_table() )
        {
            std::vector<descriptor::Type> descriptions =
                descriptor::loadCatalog( type.first, type.second->as_table() );
            for ( const descriptor::Type& desc: descriptions )
            {
                descriptor::catalog.emplace( desc.name, desc );
            }
        }

        else if ( type.second->is_value() )
        {
            auto val = type.second->as<std::string>();
            if ( !descriptor::terminalNameMap.count( val->get() ) )
            {
                throw error( "unknown type alias" ) << exception::type(type.first);
            }
            std::type_index idx = descriptor::terminalNameMap.at( val->get() );
            descriptor::terminalNameMap.emplace( type.first, idx );
            BOOST_LOG_SEV( log, severity::debug2 )
                << "loaded type alias " << type.first << " = " << val->get();
        }
        else
        {
            BOOST_LOG_SEV( log, severity::warn ) << "unused description: " << type.first;
        }
    }
    for ( const auto& type: *table )
    {
        if ( type.second->is_table() )
        {
            detector::loadCatalog( type.first, type.second->as_table() );
        }
    }
}

po::options_description load( const std::vector<fs::path>& rootPath ) {

    for ( fs::path descriptionPath: rootPath )
    {
        descriptionPath = descriptionPath / "share";
        if ( !fs::exists( descriptionPath ) )
        {
            BOOST_LOG_SEV( log, severity::debug1 )
                << "skipping description path " << descriptionPath
                << " because it does not exist";
            continue;
        }

        BOOST_LOG_SEV( log, severity::debug1 )
            << "searching " << descriptionPath << " for type descriptions";

        for ( fs::directory_entry& filename: fs::directory_iterator( descriptionPath ) )
        {
            static std::regex isDescription( R"((.+)\.toml)" );
            std::cmatch match;
            std::regex_match( filename.path().string().c_str(), match, isDescription );
            if ( match.size() )
            {
                BOOST_LOG_SEV( log, severity::debug1 ) << "loading descriptions from " << filename;
                try
                {
                    analyzeFile( filename.path() );
                }
                catch ( error& e )
                {
                    e << status::description_error;
                    e << exception::path( filename.path().string() );
                    throw;
                }
                catch ( cpptoml::parse_exception& e )
                {
                    throw polysync::error( e.what() )
                        << status::description_error
                        << exception::path( filename.path().string() );
                }
            }
        }
    }

    // For now, there actually are not any options.
    return po::options_description("Type Description Options");
}

}} // namespace polysync::toml

