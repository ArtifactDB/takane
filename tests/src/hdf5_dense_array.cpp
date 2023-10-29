#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "takane/hdf5_dense_array.hpp"

#include <numeric>
#include <string>
#include <vector>
#include <random>

template<typename ... Args_>
static void expect_error(const std::string& msg, const std::string& path, Args_&& ... args) {
    EXPECT_ANY_THROW({
        try {
            takane::hdf5_dense_array::validate(path, std::forward<Args_>(args)...);
        } catch (std::exception& e) {
            EXPECT_THAT(e.what(), ::testing::HasSubstr(msg));
            throw;
        }
    });
}

TEST(Hdf5DenseArray, Success) {
    std::string path = "TEST-hdf5_dense_array.h5";
    std::string name = "array";
    takane::hdf5_dense_array::Parameters params;
    params.dataset = name;

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        std::vector<hsize_t> dims{ 10, 20 };
        H5::DataSpace dspace(dims.size(), dims.data());
        handle.createDataSet(name, H5::PredType::NATIVE_INT, dspace);
    }

    params.dimensions = std::vector<size_t>{ 20, 10 };
    params.type = takane::array::Type::INTEGER;
    takane::hdf5_dense_array::validate(path, params);
    
    params.type = takane::array::Type::NUMBER;
    takane::hdf5_dense_array::validate(path, params);

    params.type = takane::array::Type::BOOLEAN;
    takane::hdf5_dense_array::validate(path, params);

    // Works with missing placeholders.
    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        auto dhandle = handle.openDataSet(name);
        dhandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_INT, H5S_SCALAR);
    }
    params.type = takane::array::Type::NUMBER;
    params.version = 2;
    takane::hdf5_dense_array::validate(path, params);

    // Works with strings.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        std::vector<hsize_t> dims{ 10, 20 };
        H5::DataSpace dspace(dims.size(), dims.data());
        handle.createDataSet(name, H5::StrType(0, H5T_VARIABLE), dspace);
    }
    params.type = takane::array::Type::STRING;
    takane::hdf5_dense_array::validate(path, params);
}

TEST(Hdf5DenseArray, Fails) {
    std::string path = "TEST-hdf5_dense_array.h5";
    std::string name = "array";
    takane::hdf5_dense_array::Parameters params;
    params.dataset = name;

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        handle.createGroup(name);
    }
    expect_error("expected a 'array' dataset", path, params);

    // Wrong dimensions.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        std::vector<hsize_t> dims{ 10, 20, 30 };
        H5::DataSpace dspace(dims.size(), dims.data());
        handle.createDataSet(name, H5::PredType::NATIVE_INT, dspace);
    }
    params.dimensions = std::vector<size_t>{ 30, 20 };
    expect_error("unexpected number of dimensions", path, params);

    params.dimensions.push_back(50);
    expect_error("unexpected dimension extent", path, params);

    // Wrong data type.
    params.dimensions.back() = 10;
    params.type = takane::array::Type::STRING;
    expect_error("expected a string type", path, params);

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        std::vector<hsize_t> dims{ 10, 20, 30 };
        H5::DataSpace dspace(dims.size(), dims.data());
        handle.createDataSet(name, H5::StrType(0, H5T_VARIABLE), dspace);
    }

    params.type = takane::array::Type::INTEGER;
    expect_error("expected an integer type", path, params);

    params.type = takane::array::Type::NUMBER;
    expect_error("expected an integer or floating-point type", path, params);

    // Wrong placeholder type.
    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        auto dhandle = handle.openDataSet(name);
        dhandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_INT, H5S_SCALAR);
    }
    params.type = takane::array::Type::STRING;
    expect_error("'missing-value-placeholder'", path, params);
}

TEST(Hdf5DenseArray, NameCheck) {
    std::string path = "TEST-hdf5_dense_array.h5";
    std::string name = "array";

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        std::vector<hsize_t> dims{ 10, 20 };
        H5::DataSpace dspace(dims.size(), dims.data());
        handle.createDataSet(name, H5::PredType::NATIVE_INT, dspace);
    }

    takane::hdf5_dense_array::Parameters params;
    params.dataset = name;
    params.has_dimnames = true;
    params.dimnames_group = "FOO";
    params.dimensions = std::vector<size_t>{ 20, 10 };
    expect_error("FOO", path, params);

    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        auto nhandle = handle.createGroup("FOO");

        hsize_t dim = 20;
        nhandle.createDataSet("0", H5::StrType(0, H5T_VARIABLE), H5::DataSpace(1, &dim));
        dim = 10;
        nhandle.createDataSet("1", H5::StrType(0, H5T_VARIABLE), H5::DataSpace(1, &dim));
    }
    takane::hdf5_dense_array::validate(path, params);
}
