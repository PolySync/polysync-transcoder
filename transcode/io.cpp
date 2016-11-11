#include <polysync/io.hpp>

namespace polysync { namespace plog {

std::string to_string(const node& n) {
    std::stringstream os;
    os << n;
    return os.str();
}

std::ostream& operator<<(std::ostream& os, const plog::variant& v) {
    eggs::variants::apply([&os](auto a) { os << a; }, v);
    return os;
}

}} // polysync::plog
