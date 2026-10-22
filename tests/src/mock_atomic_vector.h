#ifndef MOCK_ATOMIC_VECTOR_H
#define MOCK_ATOMIC_VECTOR_H

#include <vector>
#include <string>
#include <numeric>

#include "H5Cpp.h"
#include "utils.h"

enum class AtomicVectorType {
    INTEGER,
    NUMBER,
    STRING,
    BOOLEAN,
    VLS
};

struct AtomicVectorOptions {
public:
    AtomicVectorOptions(hsize_t length, AtomicVectorType type) : length(length), type(type), version("1.0") {
        if (type == AtomicVectorType::VLS) {
            version[2] = '1'; 
        }
    }
    AtomicVectorOptions(hsize_t length) : AtomicVectorOptions(length, AtomicVectorType::INTEGER) {}
public:
    hsize_t length;
    AtomicVectorType type;
    std::size_t string_length = 5; // set to zero for variable-length strings.
    std::string version;
};

inline H5::Group mock_atomic_vector(const std::filesystem::path& dir, const AtomicVectorOptions& options) {
    initialize_directory_simple(dir, "atomic_vector", options.version);

    H5::H5File handle(dir / "contents.h5", H5F_ACC_TRUNC);
    auto ghandle = handle.createGroup("atomic_vector");

    if (options.type == AtomicVectorType::INTEGER) {
        ghandle.createDataSet("values", H5::PredType::NATIVE_INT32, create_hdf5_dataspace(options.length));
        add_hdf5_string_attribute(ghandle, "type", "integer");

    } else if (options.type == AtomicVectorType::NUMBER) {
        ghandle.createDataSet("values", H5::PredType::NATIVE_DOUBLE, create_hdf5_dataspace(options.length));
        add_hdf5_string_attribute(ghandle, "type", "number");

    } else if (options.type == AtomicVectorType::BOOLEAN) {
        ghandle.createDataSet("values", H5::PredType::NATIVE_INT8, create_hdf5_dataspace(options.length));
        add_hdf5_string_attribute(ghandle, "type", "boolean");

    } else if (options.type == AtomicVectorType::STRING) {
        // use a fixed length string, otherwise we actually have to set the pointers.
        ghandle.createDataSet("values", create_hdf5_strtype(options.string_length), create_hdf5_dataspace(options.length)); 
        add_hdf5_string_attribute(ghandle, "type", "string");

    } else if (options.type == AtomicVectorType::VLS) {
        add_hdf5_string_attribute(ghandle, "type", "vls");

        // Just make up whatever for the heap and pointers here.
        const std::string heap = "abcdefghijklmno";
        std::vector<std::uint8_t> buffer(heap.size());
        std::copy(heap.begin(), heap.end(), reinterpret_cast<char*>(buffer.data()));
        add_hdf5_numeric_dataset(ghandle, "heap", H5::PredType::NATIVE_UINT8, buffer);

        std::vector<ritsuko::cvls::Pointer<std::uint64_t, std::uint64_t> > pointers(options.length);
        for (hsize_t i = 0; i < options.length; ++i) {
            pointers[i].offset = i % heap.size();
            pointers[i].length = sanisizer::min((i + 7) % heap.size(), heap.size() - pointers[i].offset);
        }

        auto ptype = ritsuko::cvls::define_pointer_datatype<std::uint64_t, std::uint64_t>();
        auto phandle = ghandle.createDataSet("pointers", ptype, create_hdf5_dataspace(options.length));
        phandle.write(pointers.data(), ptype);
    }

    return ghandle;
}

#endif
