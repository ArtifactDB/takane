#ifndef ATOMIC_VECTOR_H
#define ATOMIC_VECTOR_H

#include <vector>
#include <string>
#include <numeric>

#include "H5Cpp.h"
#include "utils.h"

namespace atomic_vector {

enum class Type {
    INTEGER,
    NUMBER,
    STRING,
    BOOLEAN
};

inline void mock(const std::filesystem::path& path, hsize_t length, Type type) {
    initialize_directory_simple(path, "atomic_vector", "1.0");
    H5::H5File handle(path / "contents.h5", H5F_ACC_TRUNC);
    auto ghandle = handle.createGroup("atomic_vector");

    H5::StrType stype(0, H5T_VARIABLE);

    if (type == Type::INTEGER) {
        std::vector<int> dump(length);
        std::iota(dump.begin(), dump.end(), 0);
        hdf5_utils::spawn_numeric_data(ghandle, "values", H5::PredType::NATIVE_INT32, dump);
        hdf5_utils::attach_attribute(ghandle, "type", "integer");

    } else if (type == Type::NUMBER) {
        std::vector<double> dump(length);
        std::iota(dump.begin(), dump.end(), 0.5);
        hdf5_utils::spawn_numeric_data(ghandle, "values", H5::PredType::NATIVE_DOUBLE, dump);
        hdf5_utils::attach_attribute(ghandle, "type", "number");

    } else if (type == Type::BOOLEAN) {
        std::vector<int> dump(length);
        for (hsize_t i = 0; i < length; ++i) {
            dump[i] = i % 2;
        }
        hdf5_utils::spawn_numeric_data(ghandle, "values", H5::PredType::NATIVE_INT8, dump);
        hdf5_utils::attach_attribute(ghandle, "type", "boolean");

    } else if (type == Type::STRING) {
        std::vector<std::string> raw_dump(length);
        for (hsize_t i = 0; i < length; ++i) {
            raw_dump[i] = std::to_string(i);
        }
        hdf5_utils::spawn_string_data(ghandle, "values", H5T_VARIABLE, raw_dump);
        hdf5_utils::attach_attribute(ghandle, "type", "string");
    }
}

}

#endif
