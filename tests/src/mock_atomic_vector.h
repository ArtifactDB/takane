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

inline H5::Group mock_atomic_vector(const std::filesystem::path& path, hsize_t length, AtomicVectorType type) {
    std::string version = "1.0";
    if (type == AtomicVectorType::VLS) {
        version = "1.1";
    }

    initialize_directory_simple(path, "atomic_vector", version);
    H5::H5File handle(path / "contents.h5", H5F_ACC_TRUNC);
    auto ghandle = handle.createGroup("atomic_vector");

    if (type == AtomicVectorType::INTEGER) {
        add_hdf5_dataset(ghandle, "values", H5::PredType::NATIVE_INT32, length);
        add_hdf5_attribute(ghandle, "type", "integer");

    } else if (type == AtomicVectorType::NUMBER) {
        add_hdf5_dataset(ghandle, "values", H5::PredType::NATIVE_DOUBLE, length);
        add_hdf5_attribute(ghandle, "type", "number");

    } else if (type == AtomicVectorType::BOOLEAN) {
        add_hdf5_dataset(ghandle, "values", H5::PredType::NATIVE_INT8, length);
        add_hdf5_attribute(ghandle, "type", "boolean");

    } else if (type == AtomicVectorType::STRING) {
        // use a fixed length string, otherwise we actually have to set the pointers.
        add_hdf5_dataset(ghandle, "values", H5::StrType(0, 5), length); 
        add_hdf5_attribute(ghandle, "type", "string");

    } else if (type == AtomicVectorType::VLS) {
        add_hdf5_attribute(ghandle, "type", "vls");

        // Just make up whatever for the heap and pointers here.
        const std::string heap = "abcdefghijklmno";
        std::vector<std::uint8_t> buffer(heap.size());
        std::copy(heap.begin(), heap.end(), reinterpret_cast<char*>(buffer.data()));
        auto hhandle = add_hdf5_dataset(ghandle, "heap", H5::PredType::NATIVE_UINT8, heap.size());
        hhandle.write(buffer.data(), H5::PredType::NATIVE_UINT8);

        std::vector<ritsuko::cvls::Pointer<uint64_t, std::uint64_t> > pointers(length);
        for (hsize_t i = 0; i < length; ++i) {
            pointers[i].offset = i % heap.size();
            pointers[i].length = sanisizer::min((i + 7) % heap.size(), heap.size() - pointers[i].offset);
        }

        auto ptype = ritsuko::cvls::define_pointer_datatype<std::uint64_t, std::uint64_t>();
        auto phandle = add_hdf5_dataset(ghandle, "pointers", ptype, length);
        phandle.write(pointers.data(), ptype);
    }

    return ghandle;
}

#endif
