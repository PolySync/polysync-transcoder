#include <regex>

#include <boost/make_shared.hpp>

#include <polysync/plugin.hpp>
#include <polysync/print_tree.hpp>

namespace polysync { namespace filter {

using logging::severity;
using logging::logger;

static logger log( "typefilter" );

struct typefilter : filter::plugin {

    po::options_description options() const override {

        po::options_description filter_opt( "typefilter" );
        filter_opt.add_options()
            ( "type", po::value<std::vector<std::string>>()->
              default_value(std::vector<std::string>()), "list of types to process" )
            ;

        return filter_opt;
    }

    filter::type predicate( const po::variables_map& cmdline_args) const override {
        // This functionality is implemented right now in the encoders.

        // Return empty filter; this will be thrown out in main()
        return filter::type();
    }

};

boost::shared_ptr<filter::plugin> create_typefilter() {
    return boost::make_shared<typefilter>();
}

}} // namespace polysync::plugin

BOOST_DLL_ALIAS(polysync::filter::create_typefilter, typefilter_plugin)
