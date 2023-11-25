#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "takane/takane.hpp"

#include "dense_array.h"
#include "utils.h"

#include <string>
#include <vector>
#include <filesystem>
#include <stdexcept>

struct DenseArrayTest : public ::testing::Test {
    DenseArrayTest() {
        dir = "TEST_dense_array";
        name = "dense_array";
    }

    std::filesystem::path dir;
    std::string name;

    H5::H5File reopen() {
        return H5::H5File(dir / "array.h5", H5F_ACC_RDWR);
    }

    void expect_error(const std::string& msg) {
        EXPECT_ANY_THROW({
            try {
                takane::validate(dir);
            } catch (std::exception& e) {
                EXPECT_THAT(e.what(), ::testing::HasSubstr(msg));
                throw;
            }
        });
    }
};

TEST_F(DenseArrayTest, Basics) {
    {
        initialize_directory(dir, name);
        H5::H5File handle(dir / "array.h5", H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup(name);
        hdf5_utils::attach_attribute(ghandle, "version", "2.0");
    }
    expect_error("unsupported version");

    // Success!
    {
        dense_array::mock(dir, dense_array::Type::INTEGER, { 10, 20 });
    }
    takane::validate(dir);
    EXPECT_EQ(takane::height(dir), 10);
}

TEST_F(DenseArrayTest, TypeChecks) {
    {
        dense_array::mock(dir, dense_array::Type::INTEGER, { 10, 20 });
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        ghandle.removeAttr("type");
        hdf5_utils::attach_attribute(ghandle, "type", "string");
    }
    expect_error("string datatype class");

    {
        dense_array::mock(dir, dense_array::Type::NUMBER, { 10, 20 });
    }
    takane::validate(dir);

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        ghandle.removeAttr("type");
        hdf5_utils::attach_attribute(ghandle, "type", "integer");
    }
    expect_error("32-bit signed integer");

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        ghandle.removeAttr("type");
        hdf5_utils::attach_attribute(ghandle, "type", "boolean");
    }
    expect_error("32-bit signed integer");

    {
        dense_array::mock(dir, dense_array::Type::STRING, { 10, 20 });
    }
    takane::validate(dir);

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        ghandle.removeAttr("type");
        hdf5_utils::attach_attribute(ghandle, "type", "number");
    }
    expect_error("64-bit float");
}

TEST_F(DenseArrayTest, StringContents) {
    {
        initialize_directory(dir, name);
        H5::H5File handle(dir / "array.h5", H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup(name);
        hdf5_utils::attach_attribute(ghandle, "version", "1.0");

        std::vector<hsize_t> dims { 10, 20 };
        H5::DataSpace dspace(dims.size(), dims.data());
        ghandle.createDataSet("data", H5::StrType(0, H5T_VARIABLE), dspace);
        hdf5_utils::attach_attribute(ghandle, "type", "string");
    }
    expect_error("NULL pointer");

    // To check correct iteration, our strategy is to add a NULL pointer at every corner,
    // turn the buffer size down, and verify that everything is touched.
    std::vector<hsize_t> dims { 10, 20, 5 };
    const char* dummy = "Aaron";
    takane::Options options;
    options.hdf5_buffer_size = dims.back();

    H5::DataSpace dspace(dims.size(), dims.data());
    for (size_t i = 0; i < 2; ++i) {
        for (size_t j = 0; j < 2; ++j) {
            for (size_t k = 0; k < 2; ++k) {
                std::vector<const char*> ptrs(1000, dummy);

                // Note that we go in reverse order of dimensions as HDF5 stores the fastest-changing dimension last.
                size_t multiplier = 1;
                size_t offset = k * (dims[2] - 1) * multiplier;
                multiplier *= dims[2];
                offset += j * (dims[1] - 1) * multiplier;
                multiplier *= dims[1];
                offset += i * (dims[0] - 1) * multiplier;

                ptrs[offset] = NULL;

                {
                    auto handle = reopen();
                    auto ghandle = handle.openGroup(name);
                    ghandle.unlink("data");
                    auto dhandle = ghandle.createDataSet("data", H5::StrType(0, H5T_VARIABLE), dspace);
                    dhandle.write(ptrs.data(), H5::StrType(0, H5T_VARIABLE));
                }

                EXPECT_ANY_THROW({
                    try {
                        takane::validate(dir, options);
                    } catch (std::exception& e) {
                        EXPECT_THAT(e.what(), ::testing::HasSubstr("NULL pointer"));
                        throw;
                    }
                });
            }
        }
    }
}

TEST_F(DenseArrayTest, MissingPlaceholder) {
    {
        dense_array::mock(dir, dense_array::Type::INTEGER, { 10, 20 });
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        auto dhandle = ghandle.openDataSet("data");
        hdf5_utils::attach_attribute(dhandle, "missing-value-placeholder", "NA");
    }
    expect_error("same type as its dataset");

    {
        dense_array::mock(dir, dense_array::Type::STRING, { 10, 20 });
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        auto dhandle = ghandle.openDataSet("data");
        hdf5_utils::attach_attribute(dhandle, "missing-value-placeholder", "NA");
    }
    takane::validate(dir);
}

TEST_F(DenseArrayTest, Names) {
    {
        dense_array::mock(dir, dense_array::Type::INTEGER, { 10, 20 });
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        auto nhandle = ghandle.createGroup("names");
        hdf5_utils::spawn_data(nhandle, "0", 20, H5::StrType(0, 5));
    }
    expect_error("same length as the extent");

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        auto nhandle = ghandle.openGroup("names");
        nhandle.unlink("0");
        hdf5_utils::spawn_data(nhandle, "1", 20, H5::StrType(0, 5));
    }
    takane::validate(dir);
}

TEST_F(DenseArrayTest, Transposed) {
    {
        dense_array::mock(dir, dense_array::Type::INTEGER, { 10, 20 });
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        hdf5_utils::attach_attribute(ghandle, "transposed", "123123");
    }
    expect_error("32-bit signed integer");

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        ghandle.removeAttr("transposed");
        hsize_t foo = 10;
        H5::DataSpace dspace(1, &foo);
        ghandle.createAttribute("transposed", H5::PredType::NATIVE_INT32, dspace);
    }
    expect_error("scalar");

    {
        dense_array::mock(dir, dense_array::Type::INTEGER, { 10, 20 });
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        ghandle.createAttribute("transposed", H5::PredType::NATIVE_INT32, H5S_SCALAR);
    }
    takane::validate(dir);
    EXPECT_EQ(takane::height(dir), 10);

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        auto ahandle = ghandle.openAttribute("transposed");
        int val = 1;
        ahandle.write(H5::PredType::NATIVE_INT, &val);
    }
    takane::validate(dir);
    EXPECT_EQ(takane::height(dir), 20);
}
