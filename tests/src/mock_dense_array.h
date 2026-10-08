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

inline H5::Group mock_dense_array(const std::filesystem::path& dir, DenseArrayType type, const std::vector<hsize_t>& dims) {
    std::string version = "1.0";
    if (type == DenseArrayType::VLS) {
        version = "1.1";
    }

    initialize_directory_simple(dir, "dense_array", version);
    H5::H5File handle(dir / "array.h5", H5F_ACC_TRUNC);
    auto ghandle = handle.createGroup("dense_array");

    H5::DataSpace dspace(dims.size(), dims.data());
    if (type == DenseArrayType::INTEGER) {
        ghandle.createDataSet("data", H5::PredType::NATIVE_INT32, dspace);
        add_hdf5_string_attribute(ghandle, "type", "integer");

    } else if (type == DenseArrayType::NUMBER) {
        ghandle.createDataSet("data", H5::PredType::NATIVE_DOUBLE, dspace);
        add_hdf5_string_attribute(ghandle, "type", "number");

    } else if (type == DenseArrayType::BOOLEAN) {
        ghandle.createDataSet("data", H5::PredType::NATIVE_INT8, dspace);
        add_hdf5_string_attribute(ghandle, "type", "boolean");

    } else if (type == DenseArrayType::STRING) {
        ghandle.createDataSet("data", H5::StrType(0, 10), dspace);
        add_hdf5_string_attribute(ghandle, "type", "string");

    } else if (type == DenseArrayType::VLS) {
        add_hdf5_string_attribute(ghandle, "type", "vls");

        // Just make up whatever for the heap and pointers here.
        std::string heap = "supercagifragilisticexpialadocious";
        std::vector<std::uint8_t> buffer(heap.size());
        std::copy(heap.begin(), heap.end(), reinterpret_cast<char*>(buffer.data()));
        add_hdf5_numeric_dataset(ghandle, "heap", H5::PredType::NATIVE_UINT8, buffer);

        hsize_t length = 1;
        for (auto dm : dims) {
            length *= dm;
        }
        std::vector<ritsuko::cvls::Pointer<std::uint64_t, std::uint64_t> > pointers(length);
        for (hsize_t i = 0; i < length; ++i) {
            pointers[i].offset = (i + 20) % heap.size();
            pointers[i].length = sanisizer::min((i + 1) % heap.size(), heap.size() - pointers[i].offset);
        }

        auto ptype = ritsuko::cvls::define_pointer_datatype<std::uint64_t, std::uint64_t>();
        auto phandle = ghandle.createDataSet("pointers", ptype, H5::DataSpace(dims.size(), dims.data()));
        phandle.write(pointers.data(), ptype);
    }

    return ghandle;
}

#endif
