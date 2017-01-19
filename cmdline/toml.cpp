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

std::shared_ptr<cpptoml::table> parseTomlFile( const fs::path& filename )
{
    return cpptoml::parse_file( filename.string() );
}

void loadDescriptions( const fs::path& filename )
{
    BOOST_LOG_SEV( log, severity::debug1 ) << "loading descriptions from " << filename;

    auto tablePtr = parseTomlFile( filename );

    for ( const auto& keyValuePair: *tablePtr )
    {
        std::string typeName;
        std::shared_ptr<cpptoml::base> element;
        std::tie( typeName, element ) = keyValuePair;

        if ( element->is_table() )
        {
            auto catalog = descriptor::loadCatalog( typeName, element );
            for ( auto description: catalog )
            {
                BOOST_LOG_SEV( log, severity::debug2 ) << "loading " << description.name;
                descriptor::catalog.emplace( description.name, description );
            }
        }

        else if ( element->is_value() )
        {
            std::string value = element->as<std::string>()->get();
            auto typePtr = descriptor::terminalNameMap.find( value );
            if ( typePtr == descriptor::terminalNameMap.end() )
            {
                throw error( "unknown type alias" ) << exception::type( typeName );
            }
            descriptor::terminalNameMap.emplace( typeName, typePtr->second );
            BOOST_LOG_SEV( log, severity::debug2 )
                << "loaded type alias " << typeName << " = " << value;
        }
        else
        {
            BOOST_LOG_SEV( log, severity::warn ) << "invalid description: " << typeName;
        }
    }
}

void loadDetectors( const fs::path& filename )
{
    BOOST_LOG_SEV( log, severity::debug1 ) << "loading detectors from " << filename;

    auto tablePtr = toml::parseTomlFile( filename );

    for ( const auto& keyValuePair: *tablePtr )
    {
        std::string typeName;
        std::shared_ptr<cpptoml::base> element;
        std::tie( typeName, element ) = keyValuePair;

        detector::loadCatalog( typeName, element );
    }
}

void foldPath( fs::path descriptionPath, std::function<void (const fs::path&)> call )
{
    for ( fs::directory_entry& filename: fs::directory_iterator( descriptionPath ) )
    {
        static std::regex isDescription( R"((.+)\.toml)" );
        std::cmatch match;
        std::regex_match( filename.path().string().c_str(), match, isDescription );
        if ( match.size() )
        {
            try
            {
                call( filename.path() );
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

po::options_description load( const std::vector<fs::path>& rootPaths )
{
    std::vector<fs::path> paths;

    for ( fs::path descriptionPath: rootPaths )
    {
        for ( std::string subdir: { "share", "polysync-transcoder" } )
        {
            if ( fs::exists( descriptionPath / subdir ) )
            {
                descriptionPath /= subdir;
            }
        }
        if ( !fs::exists( descriptionPath ) )
        {
            BOOST_LOG_SEV( log, severity::debug1 )
                << "skipping description path " << descriptionPath
                << " because it does not exist";
            continue;
        }
        paths.push_back( descriptionPath );
        BOOST_LOG_SEV( log, severity::debug1 )
            << "searching " << descriptionPath << " for type descriptions";
    }

    // Load *all* of the descriptions first, because the detectors need the descriptors first.
    std::for_each( paths.begin(), paths.end(), []( auto p ) { foldPath( p, loadDescriptions ); });
    std::for_each( paths.begin(), paths.end(), []( auto p ) { foldPath( p, loadDetectors ); });

    // For now, there actually are not any options.
    return po::options_description("Type Description Options");
}

}} // namespace polysync::toml

