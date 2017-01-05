#include <numeric>

#include <polysync/descriptor.hpp>
#include <polysync/detector.hpp>
#include <polysync/exception.hpp>
#include <polysync/logging.hpp>
#include <polysync/print_tree.hpp>

namespace polysync {

using polysync::logging::logger;
using polysync::logging::severity;

static logger log( "detector" );

namespace detector {

using Matchlist = std::vector<std::string>;

// Matcher is a functor called from std::accumulate, so it adds a type name to
// result if it matches.  Either way, the accumulated structure is returned.

// This functor is actually rather performance sensitive, because the full
// detector catalog is iterated every time undecoded bytes are encountered.

struct Matcher {

    const node& current;

    Matchlist operator()( Matchlist& result, const detector::Type& detect )
    {
        // Parent is not even the correct type, so short circuit and fail this test early.
        if ( detect.currentType != current.name )
        {
            BOOST_LOG_SEV( log, severity::debug2 )
                << detect.nextType << " not matched: current \""
                << current.name << "\" != \"" << detect.currentType << "\"";

            return result;
        }

        Matchlist mismatches;
        polysync::tree tree = *current.target<polysync::tree>();

        // Iterate each field in the detector looking for mismatches.
        for ( auto field: detect.matchField )
        {
            // Search the tree for matching fields by name
            auto it = std::find_if( tree->begin(), tree->end(),
                    [ field ]( const node& n ) { return n.name == field.first; });

            if ( it == tree->end() )
            {
                BOOST_LOG_SEV( log, severity::debug2 )
                    << detect.nextType << " not matched: current \""
                    << detect.currentType << "\" missing field \"" << field.first << "\"";

                // Detector field is totally missing from the node; fail the test.
                return result;
            }

            if ( *it != field.second )
            {
                // Detector value mismatch.  The test will ultimately fail, but
                // keep checking the remaining fields after this one to compute
                // a maximally informative error message to help the user tweak
                // the detector catalog.
                mismatches.emplace_back( field.first );
            }
        }

        // Successful match!
        if ( mismatches.empty() )
        {
            result.emplace_back( detect.nextType );
            return result;
        }

        // The detector failed, so print a fancy message to help the user fix the
        // catalog.  Define the formatting function here statically so the following
        // boost::log macro can manage it's invocation for performance reasons.
        auto printDetails = [&]( const std::string& field )
        {
            std::stringstream os;
            os << field + ": ";
            auto it = std::find_if( tree->begin(), tree->end(),
                    [ field ]( auto& f ){ return field == f.name; });
            os << *it;
            if ( detect.matchField.count( field ) )
            {
                os << ( *it == detect.matchField.at( field ) ? " == " : " != " );
                eggs::variants::apply( [&os]( auto& v ) { os << v; }, detect.matchField.at( field ) );
            }
            else
            {
                os << " missing from detector match list";
            }
            return os.str();
        };

        BOOST_LOG_SEV( log, severity::debug2 )
            << detect.nextType << ": mismatched { "
            << std::accumulate( mismatches.begin(), mismatches.end(), std::string(),
                    [&]( const std::string& str, auto& field ) { return str + printDetails(field); })
            << " }";

        return result;
    }

};

std::string search( const node& current )
{
    try
    {
        // Iterate detector::catalog and accumlate all matching type names
        Matchlist result = std::accumulate(
                catalog.begin(), catalog.end(), Matchlist(), Matcher { current } );

        // No matches, no problem; just return raw byte array
        if ( result.empty() )
        {
            BOOST_LOG_SEV( log, severity::debug1 )
                << "type not detected, returning undecoded bytes";
            return "raw";
        }

        BOOST_LOG_SEV( log, severity::debug1 )
            << result << " matched from current \"" << current.name << "\"";

        // Too many matches. Catalog is not orthogonal and needs tweaking.
        if ( result.size() > 1 )
        {
            throw polysync::error( "non-unique detectors: " + result[0] + " and " + result[1] );
        }

        // Exactly one match! Best situation, return the unique type name.
        return result.front();

    }
    catch ( polysync::error& e )
    {
        e << exception::module( "detector" );
        e << status::description_error;
        throw;
    }
}

}} // namespace polysync::detector
