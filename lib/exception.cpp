#include <polysync/console.hpp>
#include <polysync/exception.hpp>
#include <polysync/plog/core.hpp>
#include <polysync/print_hana.hpp>
#include <polysync/print_tree.hpp>
#include <polysync/tree.hpp>

namespace polysync
{

std::ostream& operator<<( std::ostream& os, const error& e )
{
    os << e.what() << std::endl;
    if ( const std::string* module = boost::get_error_info< exception::module >( e ) )
    {
        os << "\tModule: " << format->fieldname( *module ) << std::endl;
    }
    if ( const plog::ps_log_record* record = boost::get_error_info< exception::ps_log_record >( e ) )
    {
        os << "\tRecord: " << *record << std::endl;
    }
    if ( const std::string* tpname = boost::get_error_info< exception::type >( e ) )
    {
        os << "\tType: " << format->type( *tpname ) << std::endl;
    }
    if ( const std::string* fieldname = boost::get_error_info< exception::field >( e ) )
    {
        os << "\tField: " << format->fieldname( *fieldname ) << std::endl;
    }
    if ( const std::string* path = boost::get_error_info< exception::path >( e ) )
    {
        os << "\tPath: " << format->fieldname( *path ) << std::endl;
    }
    if ( const std::string* detector = boost::get_error_info< exception::detector >( e ) )
    {
        os << "\tDetector: " << format->fieldname( *detector ) << std::endl;
    }
    if ( const Tree* tree = boost::get_error_info< exception::tree >( e ) )
    {
        os << "\tPartial Decode: " << *tree << std::endl;
    }
    return os;
}

std::string to_printable( const error& e )
{
    std::stringstream os;
    os << e;
    return os.str();
}

} // namespace polysync
