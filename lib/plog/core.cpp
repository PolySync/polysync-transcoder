#include <typeindex>

#include <polysync/hana_descriptor.hpp>
#include <polysync/size.hpp>
#include <polysync/plog/core.hpp>

namespace polysync { namespace plog {

void load() {
    descriptor::typemap.emplace ( typeid(msg_header), 
        	    descriptor::terminal { "msg_header", size<msg_header>::value() } );
    // descriptor::typemap.emplace ( typeid(log_header), 
    //     	    descriptor::terminal { "log_header", size<log_header>::value() } );
    descriptor::typemap.emplace ( typeid(log_record), 
        	    descriptor::terminal { "log_record", size<log_record>::value() } );
    // descriptor::typemap.emplace ( typeid(log_module), 
    //     	    descriptor::terminal { "log_module", size<log_module>::value() } );
    // descriptor::catalog.emplace( "log_module", descriptor::describe<log_module>::type() );
    // descriptor::catalog.emplace( "log_header", descriptor::describe<log_header>::type() );
    descriptor::catalog.emplace( "msg_header", descriptor::describe<msg_header>::type() );
    descriptor::catalog.emplace( "log_record", descriptor::describe<log_record>::type() );
}

}} // namespace polysync::plog
