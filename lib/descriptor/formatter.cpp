#include <iostream>

#include <eggs/variant.hpp>

#include <polysync/descriptor/formatter.hpp>
#include <polysync/print_tree.hpp>

namespace polysync { namespace descriptor {

std::map< std::string, decltype( Field::format ) > formatFunction {
    { "hex", [](const variant& n)
        {
            // The hex formatter just uses the default operator<<(), but kicks on std::hex first.
            std::stringstream os;
            eggs::variants::apply([&os](auto v) { os << std::hex << "0x" << v << std::dec; }, n);
            return os.str();
        }
    },
        // Add additional formatters thusly:
    // { "NTP64", []( const variant& n )
    //     {
    //     }
    // },
};

}} // namespace polysync::descriptor
