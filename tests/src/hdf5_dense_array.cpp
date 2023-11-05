#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "takane/hdf5_dense_array.hpp"

#include <numeric>
#include <string>
#include <vector>
#include <random>

struct Hdf5DenseArrayTest : public ::testing::TestWithParam<int> {
    Hdf5DenseArrayTest() {
        path = "TEST-hdf5_dense_array.h5";
        name = "array";
    }

    std::string path, name;

public:
    static void attach_version(H5::DataSet& handle, const std::string& version) {
        H5::StrType stype(0, H5T_VARIABLE);
        auto ahandle = handle.createAttribute("version", stype, H5S_SCALAR);
        ahandle.write(stype, version);
    }

    static void attach_type(H5::DataSet& handle, const std::string& type) {
        H5::StrType stype(0, H5T_VARIABLE);
        auto ahandle = handle.createAttribute("type", stype, H5S_SCALAR);
        ahandle.write(stype, type);
    }

    template<typename ... Args_>
    static void expect_error(const std::string& msg, const std::string& path, Args_&& ... args) {
        EXPECT_ANY_THROW({
            try {
                takane::hdf5_dense_array::validate(path.c_str(), std::forward<Args_>(args)...);
            } catch (std::exception& e) {
                EXPECT_THAT(e.what(), ::testing::HasSubstr(msg));
                throw;
            }
        });
    }
};

TEST_P(Hdf5DenseArrayTest, Success) {
    auto version = GetParam();
    takane::hdf5_dense_array::Parameters params(name, { 20, 10 });
    params.version = version;

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        std::vector<hsize_t> dims{ 10, 20 };
        H5::DataSpace dspace(dims.size(), dims.data());
        auto xhandle = handle.createDataSet(name, H5::PredType::NATIVE_INT, dspace);
        if (version >= 3) {
            attach_version(xhandle, "1.0");
            attach_type(xhandle, "integer");
        }
    }
    params.type = takane::array::Type::INTEGER;
    takane::hdf5_dense_array::validate(path.c_str(), params);

    if (version >= 3) {
        H5::H5File handle(path, H5F_ACC_RDWR);
        auto xhandle = handle.openDataSet(name);
        xhandle.removeAttr("type");
        attach_type(xhandle, "number");
    }
    params.type = takane::array::Type::NUMBER;
    takane::hdf5_dense_array::validate(path.c_str(), params);

    if (version >= 3) {
        H5::H5File handle(path, H5F_ACC_RDWR);
        auto xhandle = handle.openDataSet(name);
        xhandle.removeAttr("type");
        attach_type(xhandle, "boolean");
    }
    params.type = takane::array::Type::BOOLEAN;
    takane::hdf5_dense_array::validate(path.c_str(), params);

    // Works with missing placeholders.
    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        auto dhandle = handle.openDataSet(name);
        dhandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_INT, H5S_SCALAR);
    }
    takane::hdf5_dense_array::validate(path.c_str(), params);

    // Works with strings.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        std::vector<hsize_t> dims{ 10, 20 };
        H5::DataSpace dspace(dims.size(), dims.data());
        auto xhandle = handle.createDataSet(name, H5::StrType(0, H5T_VARIABLE), dspace);
        if (version >= 3) {
            attach_version(xhandle, "1.0");
            attach_type(xhandle, "string");
        }
    }
    params.type = takane::array::Type::STRING;
    takane::hdf5_dense_array::validate(path.c_str(), params);
}

TEST_P(Hdf5DenseArrayTest, GeneralFails) {
    auto version = GetParam();
    takane::hdf5_dense_array::Parameters params(name, {});
    params.version = version;

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
        auto xhandle = handle.createDataSet(name, H5::PredType::NATIVE_INT, dspace);
        if (version >= 3) {
            attach_version(xhandle, "1.0");
            attach_type(xhandle, "integer");
        }
    }
    params.dimensions = std::vector<size_t>{ 30, 20 };
    expect_error("unexpected number of dimensions", path, params);

    params.dimensions.push_back(50);
    expect_error("unexpected dimension extent", path, params);

    // Wrong data type.
    params.dimensions.back() = 10;
    params.type = takane::array::Type::STRING;
    if (version < 3) {
        expect_error("expected a string type class", path, params);
    } else {
        expect_error("expected 'type' attribute", path, params);
    }
}

TEST_P(Hdf5DenseArrayTest, NameCheck) {
    auto version = GetParam();
    takane::hdf5_dense_array::Parameters params(name, { 20, 10 });
    params.version = version;

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        std::vector<hsize_t> dims{ 10, 20 };
        H5::DataSpace dspace(dims.size(), dims.data());
        auto xhandle = handle.createDataSet(name, H5::PredType::NATIVE_INT, dspace);
        if (version >= 3) {
            attach_version(xhandle, "1.0");
            attach_type(xhandle, "integer");
        }
    }

    if (version < 3) {
        params.dimnames_group = "FOO";
        expect_error("FOO", path, params);

        {
            H5::H5File handle(path, H5F_ACC_RDWR);
            auto nhandle = handle.createGroup("FOO");
            hsize_t dim = 20;
            nhandle.createDataSet("0", H5::StrType(0, H5T_VARIABLE), H5::DataSpace(1, &dim));
            dim = 10;
            nhandle.createDataSet("1", H5::StrType(0, H5T_VARIABLE), H5::DataSpace(1, &dim));
        }
        takane::hdf5_dense_array::validate(path.c_str(), params);

    } else {
        H5::StrType stype(0, H5T_VARIABLE);
        const char* empty = "";
        std::vector<const char*> buffer { empty, empty };

        {
            H5::H5File handle(path, H5F_ACC_RDWR);
            auto dhandle = handle.openDataSet(name);
            hsize_t ndims = 2;
            H5::DataSpace attspace(1, &ndims);
            auto ahandle = dhandle.createAttribute("dimension-names", stype, attspace);
            ahandle.write(stype, buffer.data());
        }
        takane::hdf5_dense_array::validate(path.c_str(), params);
    }
}

INSTANTIATE_TEST_SUITE_P(
    Hdf5DenseArray,
    Hdf5DenseArrayTest,
    ::testing::Values(1,2,3) // versions
);
