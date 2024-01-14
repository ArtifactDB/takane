#ifndef DELAYED_ARRAY_H
#define DELAYED_ARRAY_H

#include <vector>
#include <string>
#include <numeric>

#include "H5Cpp.h"
#include "utils.h"
#include "dense_array.h"

namespace delayed_array {

inline void mock(const std::filesystem::path& dir, dense_array::Type type, std::vector<hsize_t> dims) {
    initialize_directory_simple(dir, "delayed_array", "1.0");
    H5::H5File handle(dir / "array.h5", H5F_ACC_TRUNC);
    auto ghandle = handle.createGroup("delayed_array");
    hdf5_utils::attach_attribute(ghandle, "delayed_type", "array");
    hdf5_utils::attach_attribute(ghandle, "delayed_array", "custom takane seed array");
    hdf5_utils::attach_attribute(ghandle, "delayed_version", "1.1");

    auto dhandle = hdf5_utils::spawn_data(ghandle, "dimensions", dims.size(), H5::PredType::NATIVE_UINT32);
    dhandle.write(dims.data(), H5::PredType::NATIVE_HSIZE);

    H5::StrType stype(0, H5T_VARIABLE);
    auto thandle = ghandle.createDataSet("type", stype, H5S_SCALAR);

    std::string etype;
    if (type == dense_array::Type::INTEGER) {
        etype = "INTEGER";
    } else if (type == dense_array::Type::NUMBER) {
        etype = "FLOAT";
    } else if (type == dense_array::Type::BOOLEAN) {
        etype = "BOOLEAN";
    } else if (type == dense_array::Type::STRING) {
        etype = "STRING";
    }
    thandle.write(etype, stype);

    auto ihandle = ghandle.createDataSet("index", H5::PredType::NATIVE_UINT8, H5S_SCALAR);
    int val = 0;
    ihandle.write(&val, H5::PredType::NATIVE_INT);

    auto seed_path = dir / "seeds";
    std::filesystem::create_directory(seed_path);
    dense_array::mock(seed_path / "0", type, std::move(dims));
}

}

#endif
