#include <polysync/transcode/plugin.hpp>
#include <polysync/transcode/core.hpp>
#include <polysync/transcode/writer.hpp>
#include <hdf5.h>
#include <hdf5_hl.h>
#include <boost/filesystem.hpp>

namespace po = boost::program_options;
namespace fs = boost::filesystem;

namespace polysync { namespace transcode { namespace hdf5 {

// Map type name strings to the HDF type system.  The constant ones are
// initialized here, but custom types built from self described plog will
// appear in this list as they are discovered.

std::map<std::string, hid_t> hdf_type {
    { "uint32", H5T_NATIVE_UINT32 },
    { "uint64", H5T_NATIVE_UINT64 },
    { "ps_guid", H5T_NATIVE_UINT64 },
};

// Use the standard C interface for HDF5; I think the C++ interface is 90's
// vintage C++ which is still basically C and just a thin and useless wrapper.

class writer {
    public:
        writer(const fs::path& path) {
            file = H5Fcreate(path.string().c_str(), H5F_ACC_TRUNC, H5P_DEFAULT, H5P_DEFAULT);
            type_group = H5Gcreate(file, "/type", H5P_DEFAULT, H5P_DEFAULT, H5P_DEFAULT);
            const hsize_t dim[1] = { 8 };
            hdf_type.emplace("ps_timestamp", H5Tarray_create(H5T_NATIVE_UINT8, 1, dim));
            build_type("msg_header", plog::describe<plog::msg_header>());
            build_type("log_record", plog::describe<plog::log_record>());
        }

        void build_type(const std::string name, const std::vector<plog::field_descriptor>& desc) {
            std::streamoff size = std::accumulate(desc.begin(), desc.end(), 0, [name](auto off, auto field) { 
                    if (plog::dynamic_typemap.count(field.type) == 0)
                        std::cerr << "no typemap description for \"" + field.type + "\" (" + name + "::" + field.name + ")" << std::endl;
                    else
                        off += plog::dynamic_typemap.at(field.type).size; 
                    return off;
                    });

            hid_t mt = H5Tcreate(H5T_COMPOUND, size);
            hid_t ft = H5Tcreate(H5T_COMPOUND, size);
            std::accumulate(desc.begin(), desc.end(), 0, [this, ft, mt, name](auto off, auto field) {
                    if (hdf_type.count(field.type) == 0)
                        throw std::runtime_error("no hdf_type defined for " + name + "::" + field.name + " (type " + field.type + ")");
                    auto tp = hdf_type.at(field.type);
                    if (plog::dynamic_typemap.count(field.type)) {
                        plog::atom_description ad = plog::dynamic_typemap.at(field.type);
                        H5Tinsert(mt, field.name.c_str(), off, tp);
                        H5Tinsert(ft, field.name.c_str(), off, tp);
                        std::cout << "insert " << name << "::" << field.name << " " << off << " " << H5Tget_size(tp) << std::endl;
                        off += ad.size;
                    }
                    return off;
                    });

            // H5Tcommit1(type_group, name.c_str(), ft);
            filetype.emplace(name, ft);
            hdf_type.emplace(name, ft);
            std::cerr << "created custom type " << name << std::endl;
        }

    void write(const plog::log_record& rec) {
        topic_type topic { rec.msg_header.type, rec.msg_header.src_guid };
        if (msg_type_map.count(rec.msg_header.type) == 0)
            throw std::runtime_error("no msg_type_map for type " + std::to_string(rec.msg_header.type));
        std::string msg_type = msg_type_map.at(rec.msg_header.type); 
        hsize_t dims[1] = { 0 };
        if (!dset.count(topic)) {
            std::string name = "guid" + std::to_string(rec.msg_header.src_guid);
            hid_t ptable = H5PTcreate_fl(file, name.c_str(), filetype.at(msg_type), 1, -1);
            if (ptable == H5I_INVALID_HID)
                throw std::runtime_error("failed to create packet table");
            table.emplace(topic, ptable);

            name += "_ds";
            hsize_t dims[1] = {0};
            hsize_t maxdims[1] = { H5S_UNLIMITED };
            hid_t space = H5Screate_simple(1, dims, maxdims);
            hid_t dcpl = H5Pcreate(H5P_DATASET_CREATE);
            hsize_t chunk[1] = { 1 };
            H5Pset_chunk(dcpl, 1, chunk);
            hid_t dataset = H5Dcreate(file, name.c_str(), filetype.at(msg_type), space, H5P_DEFAULT, dcpl, H5P_DEFAULT);
            dset.emplace(topic, dataset);
        }

        // HDF5 wants to write out flat memory organized in the packet table.
        // Sadly, rec is not necesarily flat due to variable length sequence<>
        // fields.  So here, we must serialize a potentially non-flat rec into
        // a flat buffer.

        // The plog::log_record portion of the record *is* flat, so just copy that part over.
        // size_t sz = plog::size<plog::log_record>::packed();
        // std::vector<std::uint8_t> flat(sz);
        // std::memcpy(flat.data(), &rec, sz);
        
        std::stringstream flat;
        plog::writer w(flat);
        w.write(rec.index); // (std::uint32_t)0x12345678); // rec.index);
        w.write(rec.size); // (std::uint32_t)0x12345678); // rec.size);
        w.write(rec.prev_size); // (std::uint32_t)0x12345678); // rec.prev_size);
        std::cout << flat.str().size() << std::endl;
        w.write(rec.timestamp); // (std::uint64_t)0x1112131415161718); // rec.timestamp);
        std::cout << flat.str().size() << std::endl;
        w.write(rec.msg_header);
        std::cout << flat.str().size() << std::endl;

        // The user defined type may have one (or more?) sequences.  Iterate
        // over each field and do the right thing.
        auto desc = plog::description_map.at(msg_type);
        auto blob_it = rec.blob.begin();
        // std::insert_iterator<decltype(flat)> flat_it(flat, flat.end());
        std::vector<std::shared_ptr<std::uint8_t>> buffers;
        std::for_each(desc.begin(), desc.end(), [&w, &blob_it, &flat, &buffers](auto field) {
                return;
                if (field.type == "sequence<octet>") {
                    std::uint32_t* sz = new ((void *)&(*blob_it)) std::uint32_t;
                    std::cout << "serialize sequence " << field.name << " " << *sz << std::endl;
                    // hvl_t *varlen = new ((void *)&(*blob_it)) hvl_t;
                    hvl_t varlen;
                    varlen.len = *sz;
                    // buffers.emplace_back(new std::uint8_t[sz]);
                    varlen.p = malloc (2*(*sz)); //  * sizeof(std::uint8_t)); // (void *)&(*blob_it);
                    // varlen.p = buffers.back().get();
                    w.plog.write((char *)&varlen, sizeof(varlen));

        //             hvl_t varlen { 
        //             flat.resize(flat.size() + sizeof(hvl_t); 
                    blob_it += sizeof(*sz);
                } else if (field.type == "log_record") {
                    std::cout << "skipping log_record" << std::endl;
                } else {
                    char *ptr = (char *)&(*blob_it);
                    size_t sz = plog::size<plog::field_descriptor>(field).packed();
                    std::cout << "serialize " << field.type <<  " " << field.name << " " << sz << std::endl;
                    w.plog.write(ptr, sz);
                    blob_it += sz;
                    // w.write(ptr, blob_it);
                }
             });
        std::string buf = flat.str();
        std::cout << rec.index << " serial size " << buf.size() << " " << buffers.size() << " buffers,  msg_type " << std::to_string(rec.msg_header.type) << " " << buf.size() << std::endl;
        std::for_each(buf.begin(), buf.end() , [](std::uint8_t x){ std::cout << (std::uint16_t)x << " "; });
        std::cout << std::endl;
        
        if (H5PTappend(table.at(topic), 1, (void *)buf.c_str()) < 0) 
            throw std::runtime_error("cannot append record #" + std::to_string(rec.index));
        
        hsize_t extdims[1] = { 1 };
        H5Dset_extent(dset.at(topic), extdims);
        hid_t space = H5Dget_space(dset.at(topic));

        hsize_t start[1] = { 0 };
        hsize_t count[1] = { 1 };
        H5Sselect_hyperslab(space, H5S_SELECT_SET, start, NULL, count, NULL);
        H5Dwrite(dset.at(topic), filetype.at(msg_type), H5S_ALL, space,  H5P_DEFAULT, (void *)buf.c_str());
    }

    void close() {
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
    using topic_type = std::pair<plog::ps_msg_type, plog::ps_guid>;
    std::map<plog::ps_msg_type, std::string> msg_type_map;

    std::map<topic_type, hid_t> dset, table;
};

struct plugin : transcode::plugin { 

    plugin() {
        plog::dynamic_typemap.emplace("sequence<octet>", 
                plog::atom_description { "sequence<octet>", sizeof(hvl_t) } );
        hdf_type.emplace("sequence<octet>", H5Tvlen_create(H5T_NATIVE_UINT8));
    }

    ~plugin() {
        H5Tclose(hdf_type.at("sequence<octet>"));
    }

    po::options_description options() const {
        po::options_description opt("HDF5 Options");
        opt.add_options()
            ("name,n", po::value<fs::path>(), "filename")
            ;
        return opt;
    }

    void observe(const po::variables_map& vm, callback& call) const {

        auto w = std::make_shared<writer>(vm["name"].as<fs::path>());
        call.reader.connect([w](const plog::reader& r) { });
        call.type_support.connect([w](const plog::type_support& t) { 
                if (plog::description_map.count(t.name) == 0) {
                    std::cerr << "no description for " << t.name << std::endl;
                    return;
                    }
                w->build_type(t.name, plog::description_map.at(t.name)); 
                w->msg_type_map.emplace(t.type, t.name);
                });
        call.record.connect([w](const plog::log_record& rec) { w->write(rec); });
        call.cleanup.connect([w](const plog::reader&) { w->close(); });
    }
};

extern "C" BOOST_SYMBOL_EXPORT plugin plugin;

struct plugin plugin;

}}} // namespace polysync::transcode::hdf5


