#include <polysync/plugin.hpp>
#include <polysync/plog/encoder.hpp>
#include <polysync/plog/decoder.hpp>
#include <hdf5.h>
#include <hdf5_hl.h>
#include <boost/filesystem.hpp>
#include <typeindex>

namespace po = boost::program_options;
namespace fs = boost::filesystem;

namespace polysync { namespace transcode { namespace hdf5 {

// Map type name strings to the HDF type system.  The constant ones are
// initialized here, but custom types built from self described plog will
// appear in this list as they are discovered.

std::map<std::type_index, hid_t> hdf_type {
    { typeid(float), H5T_NATIVE_FLOAT },
    { typeid(double), H5T_NATIVE_DOUBLE },
    { typeid(std::uint32_t), H5T_NATIVE_UINT32 },
    { typeid(std::uint64_t), H5T_NATIVE_UINT64 },
    { typeid(plog::guid), H5T_NATIVE_UINT64 },
};

// Use the standard C interface for HDF5; I think the C++ interface is 90's
// vintage C++ which is still basically C and just a thin and useless wrapper.

class writer {
    public:
        writer(const fs::path& path) {
            file = H5Fcreate(path.string().c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
            type_group = H5Gcreate(file, "/type", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
            build_type("msg_header", plog::descriptor::describe<plog::msg_header>::type());
            build_type("log_record", plog::descriptor::describe<plog::log_record>::type());
        }

        // Create a custom HDF5 datatype that reflects the plog::type_description
        void build_type(const std::string name, const plog::descriptor::type& desc) {

            // Calculate the size of the new datatype, needed for H5Tcreate().
            // While we are at it, check that we have descriptions for each
            // field, and we know what that means for HDF5.
            std::streamoff size = std::accumulate(desc.begin(), desc.end(), 0, 
                    [name](std::streamoff off, const plog::descriptor::field& field) { 
                    if (plog::descriptor::typemap.count(field.type.target_type()) == 0)
                        throw polysync::error("no description") 
                            << exception::type(name) << exception::field(field.name)
                            << exception::plugin("HDF5");

                    if (hdf_type.count(field.type.target_type()) == 0)
                        throw polysync::error("no HDF description") 
                            << exception::type(name)
                            << exception::field(field.name)
                            << exception::plugin("HDF5");

                    off += plog::descriptor::typemap.at(field.type.target_type()).size; 
                    return off;
                    });
            hid_t ft = H5Tcreate(H5T_COMPOUND, size);

            // Loop over description and create a field for each in a hdf
            // compound type.  We already checked that our type maps know each
            // field, so we can assume the at() member functions will succeed.
            size_t off = 0;
            std::for_each(desc.begin(), desc.end(), [this, ft, name, &off](auto field) {
                    auto tp = hdf_type.at(field.type.target_type());
                    plog::descriptor::terminal ad = plog::descriptor::typemap.at(field.type.target_type());
                    H5Tinsert(ft, field.name.c_str(), off, tp);
                    off += ad.size;
                    });

            H5Tcommit1(type_group, name.c_str(), ft);
            // filetype.emplace(name, ft);
            // hdf_type.emplace(name, ft);
        }

    void write(const plog::log_record& record) {
        std::istringstream iss(record.blob);
        plog::decoder decode(iss);
        plog::node top = decode(record);
        plog::tree tree = *top.target<plog::tree>();
        auto it = std::find_if(tree->begin(), tree->end(), [](auto n) { return n.name == "msg_header"; });
        const plog::msg_header& msg_header = *it->target<plog::msg_header>();


        // Create a key value that distinguishes a specific datatype from a specific source.
        topic_type topic { msg_header.type, msg_header.src_guid };
        if (plog::type_support_map.count(msg_header.type) == 0)
            throw polysync::error("no msg_type_map for type " + std::to_string(msg_header.type));
        std::string msg_type = plog::type_support_map.at(msg_header.type); 

        // If this is the first time we have seen this topic, create a new dataset for it.
        if (!dset.count(topic)) {
            build_type(msg_type, plog::descriptor::catalog.at(msg_type));
            std::string name = "guid" + std::to_string(msg_header.src_guid);
            // hid_t ptable = H5PTcreate_fl(file, name.c_str(), filetype.at(msg_type), 1, -1);
            // if (ptable == H5I_INVALID_HID)
            //     throw std::runtime_error("failed to create packet table");
            // table.emplace(topic, ptable);
            // name += "_ds";

            hsize_t dims[1] = {0};
            hsize_t maxdims[1] = { H5S_UNLIMITED };
            hid_t space = H5Screate_simple(1, dims, maxdims);
            hid_t dcpl = H5Pcreate(H5P_DATASET_CREATE);
            hsize_t chunk[1] = { 1 };
            H5Pset_chunk(dcpl, 1, chunk);
            hid_t dataset = H5Dcreate(
                    file, name.c_str(), filetype.at(msg_type), space, H5P_DEFAULT, dcpl, H5P_DEFAULT);
            dset.emplace(topic, dataset);
            memspace.emplace(topic, space);

            H5Sclose(space);
            H5Pclose(dcpl);
        }

        // HDF5 wants to write out flat memory organized in the packet table.
        // Sadly, rec is not necesarily flat due to variable length sequence<>
        // fields.  So here, we must serialize a potentially non-flat rec into
        // a flat buffer.
        std::stringstream flat;
        plog::encoder w(flat);
        
        // The user defined type may have one (or more?) sequences.  Iterate
        // over each field and do the right thing.
        auto desc = plog::descriptor::catalog.at(msg_type);
        auto blob = record.blob.begin();
        std::for_each(desc.begin(), desc.end(), [&w, record, &blob](auto field) {
                if (field.name == "log_record") {
                    // core types we know at compile time, so fold using boost::hana
                    w.encode(record);
                } else if (field.name == "sequence<octet>") {
                    // sequences have variable length per record, which is a special case for HDF5 
                    std::uint32_t* sz = new ((void *)&(*blob)) std::uint32_t;
                    blob += sizeof(std::uint32_t);
                    hvl_t varlen { *sz, (void *)&(*blob) };
                    w.encode(varlen, sizeof(varlen));
                    blob += *sz;
                } else {
                    // All other fields just get serialized as a raw memory copy
                    // size_t sz = plog::size<plog::descriptor::field>(field).value();
                    // w.encode(&(*blob), sz);
                    // blob += sz;
                }
             });
        
        // Extend the dataset to accomodate the new record
        hid_t dataset = dset.at(topic);
        hsize_t dims[1] = { 0 };
        hid_t space = H5Dget_space(dataset);
        H5Sget_simple_extent_dims(space, dims, NULL);
        hsize_t offset[1] = { dims[0] };
        dims[0] += 1;
        H5Dextend(dataset, dims);

        hsize_t onedims[1] = {1};
        hsize_t maxdims[1] = { H5S_UNLIMITED };
        hsize_t count[1] = { 1 };
        hid_t mspace = H5Screate_simple(1, onedims, maxdims);

        // Configure a data space on the new extension
        space = H5Dget_space(dataset);
        herr_t status = H5Sselect_hyperslab(space, H5S_SELECT_SET, offset, NULL, count, NULL);

        std::string buf = flat.str();
        status = H5Dwrite(dataset, filetype.at(msg_type), mspace, space,  H5P_DEFAULT, (void *)buf.c_str());

        // if (H5PTappend(table.at(topic), 1, (void *)buf.c_str()) < 0) 
        //     throw std::runtime_error("cannot append record #" + std::to_string(rec.index));

        status = H5Sclose(space);
    }

    ~writer() {
        for (auto pair: table)
            H5PTclose(pair.second);
        for (auto pair: dset)
            H5PTclose(pair.second);
        for (auto pair: filetype)
            H5Tclose(pair.second);
        H5Gclose(type_group);
        H5Fclose(file);
    }

    hid_t file, type_group;
    std::map<std::string, hid_t> filetype;
    using topic_type = std::pair<plog::msg_type, plog::guid>;
    std::map<plog::msg_type, std::string> msg_type_map;
    std::map<topic_type, hid_t> dset, table, memspace;
};

std::shared_ptr<writer> hdf;

struct plugin : encode::plugin { 

    plugin() {
        // plog::descriptor::dynamic_typemap.emplace("sequence<octet>", 
        //         plog::descriptor::terminal { "sequence<octet>", sizeof(hvl_t) } );
        // hdf_type.emplace("sequence<octet>", H5Tvlen_create(H5T_NATIVE_UINT8));
    }

    ~plugin() {
        // H5Tclose(hdf_type.at(typeid(plog::sequence<plog::octet>)));
    }

    po::options_description options() const {
        po::options_description opt("HDF5 Options");
        opt.add_options()
            ("name,n", po::value<fs::path>(), "filename")
            ;
        return opt;
    }


    void connect(const po::variables_map& vm, encode::visitor& visit) const {

        fs::path path = vm["name"].as<fs::path>();

        visit.decoder.connect([path](const plog::decoder& decoder) { hdf = std::make_shared<writer>(path); });
        visit.record.connect([](const plog::log_record& rec) { hdf->write(rec); });
        visit.cleanup.connect([](const plog::decoder& decoder) { hdf.reset(); });
    }
};

extern "C" BOOST_SYMBOL_EXPORT plugin plugin;

struct plugin plugin;

}}} // namespace polysync::transcode::hdf5


