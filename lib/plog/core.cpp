#include <typeindex>

#include <polysync/hana_descriptor.hpp>
#include <polysync/size.hpp>
#include <polysync/plog/core.hpp>

namespace polysync { namespace plog {

void load()
{
    descriptor::terminalTypeMap.emplace ( typeid(ps_msg_header),
        	    descriptor::Terminal { "ps_msg_header", size<ps_msg_header>::value() } );
    descriptor::terminalTypeMap.emplace ( typeid(ps_log_record),
        	    descriptor::Terminal { "ps_log_record", size<ps_log_record>::value() } );
    descriptor::catalog.emplace( "ps_msg_header", descriptor::describe<ps_msg_header>::type("ps_msg_header") );
    descriptor::catalog.emplace( "ps_log_record", descriptor::describe<ps_log_record>::type("ps_log_record") );
}

}} // namespace polysync::plog
