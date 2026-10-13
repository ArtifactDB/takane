#ifndef DENSE_ARRAY_H
#define DENSE_ARRAY_H

#include <vector>
#include <filesystem>

#include "H5Cpp.h"
#include "utils.h"

enum class DenseArrayType {
    INTEGER,
    NUMBER,
    STRING,
    BOOLEAN,
    VLS
};

struct DenseArrayOptions {
    DenseArrayOptions(std::vector<hsize_t> dims, DenseArrayType type) : dims(std::move(dims)), type(type), version("1.0") {
        if (type == DenseArrayType::VLS) {
            version[2] = '1';
        }
    }
    DenseArrayOptions(std::vector<hsize_t> dims) : DenseArrayOptions(std::move(dims), DenseArrayType::INTEGER) {}
public:
    std::vector<hsize_t> dims;
    DenseArrayType type;
    std::size_t string_length = 3; // set to zero for variable-length strings.
    std::string version;
};

inline H5::Group mock_dense_array(const std::filesystem::path& dir, const DenseArrayOptions& options) {
    initialize_directory_simple(dir, "dense_array", options.version);
    H5::H5File handle(dir / "array.h5", H5F_ACC_TRUNC);
    auto ghandle = handle.createGroup("dense_array");

    H5::DataSpace dspace(options.dims.size(), options.dims.data());
    if (options.type == DenseArrayType::INTEGER) {
        ghandle.createDataSet("data", H5::PredType::NATIVE_INT32, dspace);
        add_hdf5_string_attribute(ghandle, "type", "integer");

    } else if (options.type == DenseArrayType::NUMBER) {
        ghandle.createDataSet("data", H5::PredType::NATIVE_DOUBLE, dspace);
        add_hdf5_string_attribute(ghandle, "type", "number");

    } else if (options.type == DenseArrayType::BOOLEAN) {
        ghandle.createDataSet("data", H5::PredType::NATIVE_INT8, dspace);
        add_hdf5_string_attribute(ghandle, "type", "boolean");

    } else if (options.type == DenseArrayType::STRING) {
        ghandle.createDataSet("data", create_hdf5_strtype(options.string_length), dspace);
        add_hdf5_string_attribute(ghandle, "type", "string");

    } else if (options.type == DenseArrayType::VLS) {
        add_hdf5_string_attribute(ghandle, "type", "vls");

        // Just make up whatever for the heap and pointers here.
        std::string heap = "supercagifragilisticexpialadocious";
        std::vector<std::uint8_t> buffer(heap.size());
        std::copy(heap.begin(), heap.end(), reinterpret_cast<char*>(buffer.data()));
        add_hdf5_numeric_dataset(ghandle, "heap", H5::PredType::NATIVE_UINT8, buffer);

        hsize_t length = 1;
        for (auto dm : options.dims) {
            length *= dm;
        }
        std::vector<ritsuko::cvls::Pointer<std::uint64_t, std::uint64_t> > pointers(length);
        for (hsize_t i = 0; i < length; ++i) {
            pointers[i].offset = (i + 20) % heap.size();
            pointers[i].length = sanisizer::min((i + 1) % heap.size(), heap.size() - pointers[i].offset);
        }

        auto ptype = ritsuko::cvls::define_pointer_datatype<std::uint64_t, std::uint64_t>();
        auto phandle = ghandle.createDataSet("pointers", ptype, dspace);
        phandle.write(pointers.data(), ptype);
    }

    return ghandle;
}

#endif
