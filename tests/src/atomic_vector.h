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

inline void mock(H5::Group& handle, hsize_t length, Type type) {
    H5::StrType stype(0, H5T_VARIABLE);
    auto attr2 = handle.createAttribute("version", stype, H5S_SCALAR);
    attr2.write(stype, std::string("1.0"));

    if (type == Type::INTEGER) {
        std::vector<int> dump(length);
        std::iota(dump.begin(), dump.end(), 0);
        hdf5_utils::spawn_numeric_data(handle, "values", H5::PredType::NATIVE_INT32, dump);
        hdf5_utils::attach_attribute(handle, "type", "integer");

    } else if (type == Type::NUMBER) {
        std::vector<double> dump(length);
        std::iota(dump.begin(), dump.end(), 0.5);
        hdf5_utils::spawn_numeric_data(handle, "values", H5::PredType::NATIVE_DOUBLE, dump);
        hdf5_utils::attach_attribute(handle, "type", "number");

    } else if (type == Type::BOOLEAN) {
        std::vector<int> dump(length);
        for (hsize_t i = 0; i < length; ++i) {
            dump[i] = i % 2;
        }
        hdf5_utils::spawn_numeric_data(handle, "values", H5::PredType::NATIVE_INT8, dump);
        hdf5_utils::attach_attribute(handle, "type", "boolean");

    } else if (type == Type::STRING) {
        std::vector<std::string> raw_dump(length);
        for (hsize_t i = 0; i < length; ++i) {
            raw_dump[i] = std::to_string(i);
        }
        hdf5_utils::spawn_string_data(handle, "values", H5T_VARIABLE, raw_dump);
        hdf5_utils::attach_attribute(handle, "type", "string");
    }
}

inline void mock(const std::filesystem::path& path, hsize_t length, Type type) {
    initialize_directory(path, "atomic_vector");
    H5::H5File handle(path / "contents.h5", H5F_ACC_TRUNC);
    auto ghandle = handle.createGroup("atomic_vector");
    mock(ghandle, length, type);
}

}

#endif
