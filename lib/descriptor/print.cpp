#include <polysync/descriptor/print.hpp>
#include <polysync/descriptor/lex.hpp>
#include <polysync/console.hpp>

namespace polysync { namespace descriptor {

std::ostream& operator<<(std::ostream& os, const Field& f) {
    return os << format->fieldname(f.name + ": ") << format->value(lex(f.type));}

std::ostream& operator<<(std::ostream& os, const Type& desc) {
    os << format->type(desc.name + ": { ");
    std::for_each(desc.begin(), desc.end(), [&os](auto f) { os << f << ", "; });
    os.seekp( -2, std::ios_base::end ); // remove that last comma
    return os << format->type(" }");
}

}} // namespace polysync::descriptor
