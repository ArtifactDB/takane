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
    BOOLEAN
};

inline void mock_atomic_vector(const std::filesystem::path& path, hsize_t length, AtomicVectorType type) {
    initialize_directory_simple(path, "atomic_vector", "1.0");
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
    }
}

#endif
