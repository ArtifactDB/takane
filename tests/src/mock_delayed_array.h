#ifndef MOCK_DELAYED_ARRAY_H
#define MOCK_DELAYED_ARRAY_H

#include <vector>
#include <string>
#include <numeric>
#include <cstddef>

#include "H5Cpp.h"
#include "utils.h"
#include "mock_dense_array.h"

struct DelayedArrayOptions {
    DelayedArrayOptions(std::vector<hsize_t> dims, DenseArrayType type) : dims(std::move(dims)), type(type), version("1.0") {}
    DelayedArrayOptions(std::vector<hsize_t> dims) : DelayedArrayOptions(std::move(dims), DenseArrayType::INTEGER) {}
public:
    std::vector<hsize_t> dims;
    DenseArrayType type;
    std::string version;
};

inline H5::Group mock_delayed_array(const std::filesystem::path& dir, const DelayedArrayOptions& options) {
    initialize_directory_simple(dir, "delayed_array", options.version);

    H5::H5File handle(dir / "array.h5", H5F_ACC_TRUNC);
    auto ghandle = handle.createGroup("delayed_array");
    add_hdf5_string_attribute(ghandle, "delayed_type", "array");
    add_hdf5_string_attribute(ghandle, "delayed_array", "custom takane seed array");
    add_hdf5_string_attribute(ghandle, "delayed_version", "1.1");

    add_hdf5_numeric_dataset(ghandle, "dimensions", H5::PredType::NATIVE_UINT32, options.dims);

    std::string etype;
    if (options.type == DenseArrayType::INTEGER) {
        etype = "INTEGER";
    } else if (options.type == DenseArrayType::NUMBER) {
        etype = "FLOAT";
    } else if (options.type == DenseArrayType::BOOLEAN) {
        etype = "BOOLEAN";
    } else if (options.type == DenseArrayType::STRING) {
        etype = "STRING";
    }
    H5::StrType stype(0, H5T_VARIABLE);
    auto thandle = ghandle.createDataSet("type", stype, H5S_SCALAR);
    thandle.write(etype, stype);

    auto ihandle = ghandle.createDataSet("index", H5::PredType::NATIVE_UINT8, H5S_SCALAR);
    int val = 0;
    ihandle.write(&val, H5::PredType::NATIVE_INT);

    auto seed_path = dir / "seeds";
    std::filesystem::create_directory(seed_path);
    mock_dense_array(seed_path / "0", DenseArrayOptions(options.dims, options.type));

    return ghandle;
}

#endif
