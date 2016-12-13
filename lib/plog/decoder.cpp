#include <polysync/plog/decoder.hpp>
#include <polysync/detector.hpp>
#include <polysync/print_hana.hpp>
#include <polysync/exception.hpp>

#include <boost/endian/arithmetic.hpp>
#include <algorithm>

namespace polysync { namespace plog {

using logging::severity;

// Kick off a decode with an implict type "log_record" and starting type
// "msg_header".  Continue reading the stream until it ends.
variant decoder::deep(const log_record& record) {

    variant result = node::from(record, "log_record");
    polysync::tree& tree = *result.target<polysync::tree>();

    record_endpos = stream.tellg() + static_cast<std::streamoff>(record.size);

    // There is no simple way to detect and enforce that a blob starts with a
    // msg_header.  Hence, we must just assume that every message is well
    // formed and starts with a msg_header.  In that case, might as well do a
    // static parse on msg_header and dynamic parse the rest.
    tree->emplace_back(node::from(decode<msg_header>(), "msg_header"));

    // Burn through the rest of the log record, decoding a sequence of types.
    while (stream.tellg() < record_endpos) {
        std::string type = detect(tree->back());
        tree->emplace_back(type, decode(type));
    }
    return std::move(result);
}

// Define a set of factory functions that know how to decode specific binary
// types.  They keys are strings from the "type" field of the TOML descriptions.

std::map<std::string, decoder::parser> decoder::parse_map = {
   // Native integers
   { "uint8", [](decoder& r) { return r.decode<std::uint8_t>(); } },
   { "uint16", [](decoder& r) { return r.decode<std::uint16_t>(); } },
   { "uint32", [](decoder& r) { return r.decode<std::uint32_t>(); } },
   { "uint64", [](decoder& r) { return r.decode<std::uint64_t>(); } },
   { "int8", [](decoder& r) { return r.decode<std::int8_t>(); } },
   { "int16", [](decoder& r) { return r.decode<std::int16_t>(); } },
   { "int32", [](decoder& r) { return r.decode<std::int32_t>(); } },
   { "int64", [](decoder& r) { return r.decode<std::int64_t>(); } },

   // Bigendian integers
   { "uint16.be", [](decoder& r){ return r.decode<boost::endian::big_uint16_t>(); } },
   { "uint32.be", [](decoder& r){ return r.decode<boost::endian::big_uint32_t>(); } },
   { "uint64.be", [](decoder& r){ return r.decode<boost::endian::big_uint64_t>(); } },
   { "int16.be", [](decoder& r){ return r.decode<boost::endian::big_int16_t>(); } },
   { "int32.be", [](decoder& r){ return r.decode<boost::endian::big_int32_t>(); } },
   { "int64.be", [](decoder& r){ return r.decode<boost::endian::big_int64_t>(); } },

   // Floating point types and aliases
   { "float", [](decoder& r) { return r.decode<float>(); } },
   { "float32", [](decoder& r) { return r.decode<float>(); } },
   { "double", [](decoder& r) { return r.decode<double>(); } },
   { "float64", [](decoder& r) { return r.decode<double>(); } },

   // Bigendian floats: first byteswap as uint, then emplacement new to float.
   { "float.be", [](decoder& r) {
           std::uint32_t swap = r.decode<boost::endian::big_uint32_t>().value();
           return *(new ((void *)&swap) float);
       } },
   { "float32.be", [](decoder& r) {
           std::uint32_t swap = r.decode<boost::endian::big_uint32_t>().value();
           return *(new ((void *)&swap) float);
       } },
   { "double.be", [](decoder& r) {
           std::uint64_t swap = r.decode<boost::endian::big_uint64_t>().value();
           return *(new ((void *)&swap) float);
       } },
   { "float64.be", [](decoder& r) {
           std::uint64_t swap = r.decode<boost::endian::big_uint64_t>().value();
           return *(new ((void *)&swap) float);
       } },

   // Fallback bytes buffer
   { "raw", [](decoder& r)
       {
           std::streampos rem = r.record_endpos - r.stream.tellg();
           polysync::bytes raw(rem);
           r.stream.read((char *)raw.data(), rem);
           return node("raw", raw);
       }},

   // PLog aliases
   { "ps_guid", [](decoder& r){ return r.decode<plog::guid>(); } },
   { "ps_timestamp", [](decoder& r) { return r.decode<plog::timestamp>(); } },
   { "NTP64.be", [](decoder& r){ return r.decode<boost::endian::big_uint64_t>(); } },
};

// Read a field, described by looking up thef type by string.  The type strings can
// be compound types described in the TOML description, primitive types known
// by parse_map, or strings generated by boost::hana from compile time structs.
variant decoder::decode( const std::string& type ) {

    auto parse = parse_map.find( type );

    if ( parse != parse_map.end() ) {
        return parse->second( *this );
    }

    if ( !descriptor::catalog.count( type ) ) {
        throw polysync::error( "no decoder" )
           << exception::type( type )
           << exception::module( "plog::decoder" )
           << status::description_error;
    }

    BOOST_LOG_SEV( log, severity::debug2 )
        << "decoding \"" << type << "\" at offset " << stream.tellg();

    const descriptor::type& desc = descriptor::catalog.at( type );
    return decode( desc );
}

struct branch_builder {
    polysync::tree branch;
    const descriptor::field& field;
    decoder* d;

    // Terminal types
    void operator()( const std::type_index& idx ) const {
        auto term = descriptor::typemap.find(idx);
        if ( term == descriptor::typemap.end() )
            throw polysync::error("no typemap") << exception::field(field.name);
        std::string tname = term->second.name + ( field.bigendian ? ".be" : "" );

        variant a = d->decode(tname);
        branch->emplace_back(field.name, a);
        branch->back().format = field.format;
        BOOST_LOG_SEV(d->log, severity::debug2) << field.name << " = "
            << field.format(branch->back()) << " (" << term->second.name
            << (field.bigendian ? ", bigendian" : "")
            << ")";
    }

    // Nested described type
    void operator()(const descriptor::nested& nest) const {
        // Type aliases sometimes appear as nested types because the alias was
        // defined after the type that uses it.  This is why we check typemap
        // here; if we could know when parsing the TOML that the type was
        // actually an alias not a nested type, then this would not be necessary.
        if (descriptor::namemap.count(nest.name))
            return operator()(descriptor::namemap.at(nest.name));

        if (!descriptor::catalog.count(nest.name))
            throw polysync::error("no nested descriptor for \"" + nest.name + "\"");
        const descriptor::type& desc = descriptor::catalog.at(nest.name);
        node a(field.name, d->decode(desc));
        branch->emplace_back(field.name, a);
        BOOST_LOG_SEV(d->log, severity::debug2) << field.name << " = " << a
            << " (nested \"" << nest.name << "\")";
    }

    // Burn off unused or reserved space
    void operator()(const descriptor::skip& skip) const {
        polysync::bytes raw(skip.size);
        d->stream.read((char *)raw.data(), skip.size);
        std::string name = "skip-" + std::to_string(skip.order);
        branch->emplace_back(name, raw);
        BOOST_LOG_SEV(d->log, severity::debug2) << name << " " << raw;
    }

    void operator()(const descriptor::array& desc) const {

        // Not sure yet if the array size is fixed, or read from a field in the parent node.
        auto sizefield = desc.size.target<std::string>();
        auto fixedsize = desc.size.target<size_t>();
        size_t size;

        if (sizefield) {

            // The branch should have a previous element with name sizefield
            auto it = std::find_if(branch->begin(), branch->end(),
                    [sizefield](const node& n) { return n.name == *sizefield; });

            if (it == branch->end())
                throw polysync::error("size indicator field not found")
                    << exception::field(*sizefield)
                    << status::description_error;

            // Figure out the size, regardless of the integer type
            std::stringstream os;
            os << *it;
            try {
                size = std::stoll(os.str());
            } catch (std::invalid_argument) {
                throw polysync::error("cannot parse size value \"" + os.str() +
                        "\", string of size " + std::to_string(os.str().size()))
                    << exception::field(*sizefield)
                    << status::description_error;
            }
        } else
            size = *fixedsize;

        auto nesttype = desc.type.target<std::string>();

        if (nesttype) {
            if (!descriptor::catalog.count(*nesttype))
                throw polysync::error("unknown nested type");

            const descriptor::type& nest = descriptor::catalog.at(*nesttype);
            std::vector<polysync::tree> array;
            for (size_t i = 0; i < size; ++i) {
                BOOST_LOG_SEV(d->log, severity::debug2) << "decoding " << nest.name
                    <<  " #" << i + 1 << " of " << size;
                array.push_back(*(d->decode(nest).target<polysync::tree>()));
            }
            branch->emplace_back(field.name, array);
            BOOST_LOG_SEV(d->log, severity::debug2) << field.name << " = " << array;
        } else {
            std::type_index idx = *desc.type.target<std::type_index>();
            std::vector<std::uint8_t> array(size);
            for(std::uint8_t& val: array)
                d->decode(val);
            branch->emplace_back(field.name, array);
            BOOST_LOG_SEV(d->log, severity::debug2) << field.name << " = " << array;
        }
    }

};

polysync::variant decoder::decode(const descriptor::type& desc) {

    polysync::tree child(desc.name);

    try {
        std::for_each(desc.begin(), desc.end(), [&](const descriptor::field& field) {
                eggs::variants::apply(branch_builder { child, field, this }, field.type);
                });
    } catch (polysync::error& e) {
        e << exception::module("plog::decoder");
        e << exception::type(desc.name);
        e << exception::tree(child);
        throw;
    } catch ( std::ios_base::failure ) {
        throw polysync::error("read error")
            << exception::module("plog::decoder")
            << exception::type(desc.name);
    }

    return std::move(node(desc.name, child));
}


}}  // polysync::plog
