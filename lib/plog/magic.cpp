#include <fstream>

#include <polysync/plog/core.hpp>
#include <polysync/plog/decoder.hpp>
#include <polysync/plog/magic.hpp>

namespace polysync { namespace plog {

bool checkMagic( std::ifstream& stream )
{
    std::streamoff startPosition = stream.tellg();

    plog::decoder decoder( stream );

    try
    {
        std::uint8_t version_major = decoder.decode<std::uint8_t>();
        std::uint8_t version_minor = decoder.decode<std::uint8_t>();
        std::uint16_t version_subminor = decoder.decode<std::uint16_t>();

        stream.seekg( startPosition );

        // These are just guesses of the future, but are way better than checking nothing.
        return version_major < 3 and
               version_minor < 10 and
               version_subminor < 100;
    }
    catch ( polysync::error )
    {
        return false;
    }
}

}} // namespace polysync::plog
