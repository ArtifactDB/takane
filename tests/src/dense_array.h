#ifndef DENSE_ARRAY_H
#define DENSE_ARRAY_H

#include <vector>
#include <string>
#include <numeric>

#include "H5Cpp.h"
#include "utils.h"

namespace dense_array {

enum class Type {
    INTEGER,
    NUMBER,
    STRING,
    BOOLEAN
};

inline void mock(const std::filesystem::path& dir, Type type, std::vector<hsize_t> dims) {
    initialize_directory_simple(dir, "dense_array", "1.0");
    H5::H5File handle(dir / "array.h5", H5F_ACC_TRUNC);
    auto ghandle = handle.createGroup("dense_array");

    H5::DataSpace dspace(dims.size(), dims.data());
    if (type == Type::INTEGER) {
        ghandle.createDataSet("data", H5::PredType::NATIVE_INT32, dspace);
        hdf5_utils::attach_attribute(ghandle, "type", "integer");

    } else if (type == Type::NUMBER) {
        ghandle.createDataSet("data", H5::PredType::NATIVE_DOUBLE, dspace);
        hdf5_utils::attach_attribute(ghandle, "type", "number");

    } else if (type == Type::BOOLEAN) {
        ghandle.createDataSet("data", H5::PredType::NATIVE_INT8, dspace);
        hdf5_utils::attach_attribute(ghandle, "type", "boolean");

    } else if (type == Type::STRING) {
        ghandle.createDataSet("data", H5::StrType(0, 10), dspace);
        hdf5_utils::attach_attribute(ghandle, "type", "string");
    }
}

}

#endif
