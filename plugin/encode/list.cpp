#include <set>

#include <boost/make_shared.hpp>

#include <polysync/plugin.hpp>
#include <polysync/descriptor/lex.hpp>
#include <polysync/descriptor/print.hpp>
#include <polysync/print_tree.hpp>

// The list plugin dumps the dataypes and the record counts for each type found in a plog.

namespace po = boost::program_options;

namespace polysync { namespace plugin {

static polysync::logging::logger log { "list" };
using polysync::logging::severity;

struct model_counter {

    std::map<std::string, size_t> types;

    // Do not count terminals or anything that is not a tree.
    template <typename T>
    void count( const T& v )
    {
    }

    // polysync::tree is the nested type that is useful inspect.
    void count( const polysync::Tree& tree )
    {
        // Initialize counter with 0 if necessary, then add one.
        types[tree.type] += 1;

        // Recurse all the nested types
        std::for_each( tree->begin(), tree->end(), [this](const Node& n) { this->operator()(n); } );
    }

    void count( const std::vector<polysync::Tree>& trees )
    {
        // vectors of trees are supposed to be homogeneous, so inspect the first one only.
        if (!trees.empty())
        {
            count(trees.front());
        }
    }

    void operator()(const polysync::Node& record)
    {
        // Unpack a variant and dispatch the appropriate counter method.
        eggs::variants::apply([this](auto v) { this->count(v); }, record);
    }

};

struct list : encode::plugin {

    po::options_description options() const override
    {
        po::options_description opt("list: Print information about a file");
        opt.add_options()
            ("detail", "print detailed type model")
            ;
        return opt;
    };

    model_counter count;

    void connect( const po::variables_map& cmdline_args, encode::Visitor& visit ) override
    {
        std::set<std::string> typefilter;
        for ( std::string type: cmdline_args["type"].as<std::vector<std::string>>() )
        {
            typefilter.insert(type);
        }

        bool detail = cmdline_args.count( "detail" );

        // As each record is visited, we only count them.
        visit.record.connect( std::ref(count) );

        // At cleanup, we finally print, because now we know how many instances we saw.
        visit.cleanup.connect( [ this, detail, typefilter ]( const Decoder& decode )
            {
                // Iterate all the found types and print them.
                for ( auto pair: count.types )
                {
                    // Honor the type filter
                    if ( !typefilter.count(pair.first) && !typefilter.empty() )
                    {
                        continue;
                    }

                    std::string cmsg = "x" + std::to_string( count.types.at(pair.first) );

                    if ( detail )
                    {
                        std::cout << format->begin_block( pair.first );

                        if ( !descriptor::catalog.count( pair.first ) )
                        {
                            throw polysync::error( "no type description" )
                                << exception::type( pair.first )
                                << status::description_error;
                        }

                        for ( const descriptor::Field& field: descriptor::catalog.at(pair.first) )
                        {
                            std::string tags;
                            if ( field.byteorder == descriptor::ByteOrder::BigEndian )
                            {
                                tags += std::string(" bigendian ");
                            }
                            std::cout << format->begin_item();
                            if ( !field.name.empty() )
                            {
                                std::cout << format->fieldname( field.name + ": " );
                            }
                            std::cout << descriptor::lex( field.type );
                            if ( !tags.empty() )
                            {
                                std::cout << " (" << tags << ")";
                            }
                            std::cout << format->end_item();
                        }
                        std::cout << format->end_block(cmsg);
                    }
                    else
                    {
                        std::cout << pair.first << " " << cmsg << std::endl;
                    }
                }
        });
    }
};

boost::shared_ptr<encode::plugin> create_list()
{
    return boost::make_shared<list>();
}

}} // namespace polysync::plugin

BOOST_DLL_ALIAS(polysync::plugin::create_list, list_plugin)

