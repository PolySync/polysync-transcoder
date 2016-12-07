#include <regex>

#include <boost/make_shared.hpp>

#include <polysync/plugin.hpp>

namespace polysync { namespace filter {

using logging::severity;
using logging::logger;

static logger log( "filter" );

struct slice : filter::plugin {

    po::options_description options() const override {

        po::options_description filter_opt( "slice" );
        filter_opt.add_options()
            ( "slice", po::value<std::string>(), "<begin:stride:end> Numpy style record slice syntax" )
            ;

        return filter_opt;
    }

    filter::type predicate( const po::variables_map& cmdline_args) const override {

        if ( cmdline_args.count("slice") ) {
            // Emulate the the excellent Python Numpy slicing syntax
            static std::regex slice_re( R"((\d+)?(:)?(\d+)?(:)?(\d+)?)" );
            std::smatch slice;
            if ( std::regex_match(cmdline_args["slice"].as<std::string>(), slice, slice_re) ) {
                if ( slice[4].matched && !slice[5].matched )
                    throw polysync::error( "bad slice format" ) << polysync::status::bad_argument;

                size_t begin = slice[1].matched ? std::stol( slice[1] ) : 0;
                size_t end = slice[2].matched ? -1 : begin + 1;
                size_t stride = slice[4].matched ? std::stol( slice[3] ) : 1;
                end = slice[5].matched ? std::stol( slice[5] ) : end;
                end = ( slice[3].matched && !slice[4].matched ) ? std::stol( slice[3] ) : end;

                BOOST_LOG_SEV( log, severity::debug2 ) << "slice " << begin << ":" << stride << ":" << end;

                return [begin, end]( const plog::log_record& rec ) {
                    return ( rec.index >= begin ) && ( rec.index < end );
                };
            }
            else
                throw polysync::error( "bad slice format" ) << polysync::status::bad_argument;
        }

        return filter::type();
    }

};

boost::shared_ptr<filter::plugin> create_slice() {
    return boost::make_shared<slice>();
}

}} // namespace polysync::plugin

BOOST_DLL_ALIAS(polysync::filter::create_slice, slice_plugin)
