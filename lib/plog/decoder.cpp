#include <polysync/detector.hpp>
#include <polysync/print_tree.hpp>
#include <polysync/print_hana.hpp>
#include <polysync/exception.hpp>
#include <polysync/hana.hpp>

#include <polysync/plog/decoder.hpp>

namespace polysync { namespace plog {

using logging::severity;

// The TOML descriptions know the type names as strings, and metadata like
// endianess that also affect how the numbers are decoded.  The decoder,
// however, needs the C++ type, not a string representation of the type.  Here,
// we have a grand old mapping of the string type names (from TOML) to the type
// specific decode function.
std::map<std::string, decoder::parser> decoder::parse_map =
{
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
};


decoder::decoder( std::istream& st ) : stream( st )
{
    // Enable exceptions for all reads
    stream.exceptions( std::ifstream::failbit );
}

iterator decoder::begin()
{
    return iterator ( this, stream.tellg() );
}

iterator decoder::end()
{
    // Find the last byte of the file
    std::streamoff current = stream.tellg();
    stream.seekg( 0, std::ios_base::end );
    std::streamoff end = stream.tellg();
    stream.seekg( current );

    return iterator ( end );
}


// Kick off a decode with an implict type "log_record" and starting type
// "msg_header".  Continue reading the stream until it ends.
variant decoder::deep(const ps_log_record& record) {

    variant result = from_hana(record, "ps_log_record");
    polysync::tree& tree = *result.target<polysync::tree>();

    record_endpos = stream.tellg() + static_cast<std::streamoff>(record.size);

    // There is no simple way to detect and enforce that a blob starts with a
    // msg_header.  Hence, we must just assume that every message is well
    // formed and starts with a msg_header.  In that case, might as well do a
    // static parse on msg_header and dynamic parse the rest.
    tree->emplace_back(from_hana(decode<ps_msg_header>(), "ps_msg_header"));

    // Burn through the rest of the log record, decoding a sequence of types.
    while (stream.tellg() < record_endpos) {
        std::string type = detector::search(tree->back());
        tree->emplace_back(type, decode(type));
    }
    return std::move(result);
}

// Read a field, described by looking up thef type by string.  The type strings can
// be compound types described in the TOML description, primitive types known
// by parse_map, or strings generated by boost::hana from compile time structs.
variant decoder::decode( const std::string& type )
{

    auto parse = parse_map.find( type );

    if ( parse != parse_map.end() )
    {
        return parse->second( *this );
    }

    if ( !descriptor::catalog.count( type ) )
    {
        throw polysync::error( "no decoder" )
           << exception::type( type )
           << exception::module( "plog::decoder" )
           << status::description_error;
    }

    BOOST_LOG_SEV( log, severity::debug2 )
        << "decoding \"" << type << "\" at offset " << stream.tellg();

    const descriptor::Type& desc = descriptor::catalog.at( type );
    return decode( desc );
}

struct branch_builder
{
    polysync::tree branch;
    const descriptor::Field& fieldDescriptor;
    decoder* d;

    // Terminal types
    void operator()( const std::type_index& idx ) const
    {
        auto term = descriptor::terminalTypeMap.find(idx);
        if ( term == descriptor::terminalTypeMap.end() )
        {
            throw polysync::error("no typemap") << exception::field(fieldDescriptor.name);
        }

        std::string tname;

        switch ( fieldDescriptor.byteorder )
        {
            case descriptor::ByteOrder::LittleEndian:
                tname = term->second.name;
                break;
            case descriptor::ByteOrder::BigEndian:
                tname = term->second.name + ".be";
                break;
        }

        variant a = d->decode(tname);
        branch->emplace_back(fieldDescriptor.name, a);
        branch->back().format = fieldDescriptor.format;
        BOOST_LOG_SEV(d->log, severity::debug2) << fieldDescriptor.name << " = "
            << branch->back() << " (" << term->second.name
            << (fieldDescriptor.byteorder == descriptor::ByteOrder::BigEndian ? ", bigendian" : "")
            << ")";
    }

    // Nested described type
    void operator()(const descriptor::Nested& nest) const {
        // Type aliases sometimes appear as nested types because the alias was
        // defined after the type that uses it.  This is why we check typemap
        // here; if we could know when parsing the TOML that the type was
        // actually an alias not a nested type, then this would not be necessary.
        if (descriptor::terminalNameMap.count(nest.name))
            return operator()(descriptor::terminalNameMap.at(nest.name));

        if (!descriptor::catalog.count(nest.name))
            throw polysync::error("no nested descriptor for \"" + nest.name + "\"");
        const descriptor::Type& desc = descriptor::catalog.at(nest.name);
        node a(fieldDescriptor.name, d->decode(desc));
        branch->emplace_back(fieldDescriptor.name, a);
        BOOST_LOG_SEV(d->log, severity::debug2) << fieldDescriptor.name << " = " << a
            << " (nested \"" << nest.name << "\")";
    }

    // Burn off unused or reserved space
    void operator()(const descriptor::Skip& skip) const
    {
        polysync::bytes raw(skip.size);
        d->stream.read((char *)raw.data(), skip.size);
        std::string name = "skip-" + std::to_string(skip.order);
        branch->emplace_back(name, raw);
        BOOST_LOG_SEV(d->log, severity::debug2) << name << " " << raw;
    }

    void operator()(const descriptor::Array& desc) const
    {
        // Not sure yet if the array size is fixed, or read from a field in the parent node.
        auto sizefield = desc.size.target<std::string>();
        auto fixedsize = desc.size.target<size_t>();
        size_t size;

        if (sizefield)
        {
            // The branch should have a previous element with name sizefield
            auto node_iter = std::find_if(branch->begin(), branch->end(),
                    [sizefield](const node& n) { return n.name == *sizefield; });

            if (node_iter == branch->end())
                throw polysync::error("array size indicator field not found")
                    << exception::field(*sizefield)
                    << status::description_error;

	    // Compute the size, regardless of the integer type, by just lexing
	    // to a string and converting the string back to an int.
	    // Otherwise, we would have to fuss with the exact integer type
	    // which is not very interesting here.
            std::stringstream os;
            os << *node_iter;
            try {
                size = std::stoll( os.str() );
            } catch ( std::invalid_argument ) {
                throw polysync::error("cannot parse array size value \"" + os.str() +
                        "\", string of size " + std::to_string(os.str().size()))
                    << exception::field(*sizefield)
                    << status::description_error;
            }
        }
        else
        {
            size = *fixedsize;
        }

        auto nesttype = desc.type.target<std::string>();

        if (nesttype)
        {
            if (!descriptor::catalog.count(*nesttype))
                throw polysync::error("unknown nested type");

            const descriptor::Type& nest = descriptor::catalog.at(*nesttype);
            std::vector<polysync::tree> array;
            for (size_t i = 0; i < size; ++i)
            {
                BOOST_LOG_SEV(d->log, severity::debug2) << "decoding " << nest.name
                    <<  " #" << i + 1 << " of " << size;
                array.push_back(*(d->decode(nest).target<polysync::tree>()));
            }
            branch->emplace_back(fieldDescriptor.name, array);
            BOOST_LOG_SEV(d->log, severity::debug2) << fieldDescriptor.name << " = " << array;
        }
        else
        {
            std::type_index idx = *desc.type.target<std::type_index>();
            std::vector<std::uint8_t> array(size);
            for(std::uint8_t& val: array)
            {
                d->decode(val);
            }
            branch->emplace_back(fieldDescriptor.name, array);
            BOOST_LOG_SEV(d->log, severity::debug2) << fieldDescriptor.name << " = " << array;
        }
    }

};

polysync::variant decoder::decode(const descriptor::Type& desc)
{
    polysync::tree child(desc.name);

    try
    {
        std::for_each(desc.begin(), desc.end(), [&]( const descriptor::Field& field ) {
                eggs::variants::apply(branch_builder { child, field, this }, field.type);
                });
    }
    catch (polysync::error& e)
    {
        e << exception::module("plog::decoder");
        e << exception::type(desc.name);
        e << exception::tree(child);
        throw;
    }
    catch ( std::ios_base::failure )
    {
        throw polysync::error("read error")
            << exception::module("plog::decoder")
            << exception::type(desc.name);
    }

    return std::move(node(desc.name, child));
}

void decoder::decode(boost::multiprecision::cpp_int& value)
{
    // 16 is correct for ps_hash_type, but this needs to be flexible if other
    // types come along that are not 128 bits.  I have not been able to
    // determine how to compute how many bits a particular value cpp_int
    // actually needs.
    bytes buf( 16 );
    stream.read( (char*)buf.data(), buf.size() );
    multiprecision::import_bits( value, buf.begin(), buf.end(), 8 );
}

// Specialize name_type because the underlying std::string type needs special
// handling.  It resembles a Pascal string (length first, no trailing zero
// as a C string would have)
void decoder::decode( ps_name_type& name )
{
    std::uint16_t len;
    stream.read( (char*)( &len ), sizeof(len) );
    name.resize( len );
    stream.read( (char*)( name.data() ), len );
}

void decoder::decode(bytes& raw)
{
    stream.read( (char*)raw.data(), raw.size() );
}

variant decoder::operator()( const descriptor::Type& type )
{
    return decode( type );
}

}}  // polysync::plog
