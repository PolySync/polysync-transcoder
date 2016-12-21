#pragma

#include <boost/hana.hpp>

#include <polysync/descriptor.hpp>
#include <polysync/exception.hpp>

namespace polysync { namespace descriptor {

namespace hana = boost::hana;

// Create a type description of a static structure, using hana for class instrospection
template <typename S, typename = std::enable_if_t<hana::Struct<S>::value> >
struct describe {

    // Static introspection of the descriptor for Struct
    static descriptor::Type type( std::string tpname ) {
        try {
	    //     if ( !terminalTypeMap.count( typeid( S ) ) ) {
		//     throw polysync::error( "unregistered type" )
		// 	    << exception::type( typeid( S).name() );
		// }

		// std::string tpname = descriptor::terminalTypeMap.at( typeid(S) ).name;

		return hana::fold( S(), descriptor::Type(tpname),
		    [tpname]( auto desc, auto pair ) {
		        std::string name = hana::to<char const*>( hana::first( pair ) );

			if ( terminalTypeMap.count( typeid( hana::second(pair) ) ) == 0 ) {
			    throw polysync::error( "missing typemap" )
			        << exception::type( tpname )
			        << exception::field( name );
			}

			Terminal a = terminalTypeMap.at( std::type_index( typeid( hana::second(pair) ) ) );
			desc.emplace_back(descriptor::Field { name, typeid( hana::second(pair) ) } );
			return desc;
			});
	    } catch ( polysync::error& e ) {
		    e << exception::module( "description" );
		    e << status::description_error;
		    throw e;
	    }
    }

    // Generate self descriptions of types
    static std::string string() {
        if ( !descriptor::terminalTypeMap.count( typeid(S) ) )
            throw polysync::error( "no typemap description" )
                        << exception::module( "description" )
                        << exception::type( typeid(S).name() )
                        << status::description_error;

        std::string tpname = descriptor::terminalTypeMap.at( typeid(S) ).name;
        std::string result = tpname + " { ";
        hana::for_each(S(), [ &result, tpname ]( auto pair ) {
                std::type_index tp = typeid(hana::second( pair ));
                std::string fieldname = hana::to<char const*>( hana::first(pair) );
                if ( descriptor::terminalTypeMap.count(tp) == 0 )
                    throw polysync::error( "type not described" )
                        << exception::type( tpname )
                        << exception::field( fieldname )
                        << exception::module( "description" )
                        << status::description_error;

                result += fieldname + ": " + descriptor::terminalTypeMap.at(tp).name + "; ";
                });
        return result + "}";
    }

};

}} // namespace polysync::descriptor
