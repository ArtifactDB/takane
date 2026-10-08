#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "takane/utils_array.hpp"

#include "utils.h"

TEST(CastArrayDimensions, Basic) {
    {
        std::vector<int> input{ 0, 10, 20 };
        auto output = takane::cast_array_dimensions<std::size_t>(input);
        EXPECT_EQ(output.size(), input.size());
        EXPECT_EQ(output[0], 0);
        EXPECT_EQ(output[1], 10);
        EXPECT_EQ(output[2], 20);
    }

    // Same type is a no-op.
    {
        std::vector<std::size_t> input{ 0, 10, 20 };
        auto output = takane::cast_array_dimensions<std::size_t>(input);
        EXPECT_EQ(input, output);
    }
}

TEST(ValidateArrayDimnames, Okay) {
    auto dir = define_test_path("utils_array");
    initialize_directory(dir);
    auto path = dir / "payload.h5";
    std::vector<std::size_t> extents{ 10, 4, 30 };

    // No dimnames at all.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
    }
    {
        H5::H5File handle(path, H5F_ACC_RDONLY);
        takane::validate_array_dimnames(handle, "dimnames", extents, {});
    }

    // Empty dimnames. 
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto dhandle = handle.createGroup("dimnames");
    }
    {
        H5::H5File handle(path, H5F_ACC_RDONLY);
        takane::validate_array_dimnames(handle, "dimnames", extents, {});
    }

    // Partial dimnames. 
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto dhandle = handle.createGroup("dimnames");
        dhandle.createDataSet("1", H5::StrType(0, 10), create_hdf5_dataspace(4));
    }
    {
        H5::H5File handle(path, H5F_ACC_RDONLY);
        takane::validate_array_dimnames(handle, "dimnames", extents, {});
    }

    // Full dimnames. 
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto dhandle = handle.createGroup("dimnames");
        dhandle.createDataSet("0", H5::StrType(0, 10), create_hdf5_dataspace(10));
        dhandle.createDataSet("1", H5::StrType(0, 10), create_hdf5_dataspace(4));
        dhandle.createDataSet("2", H5::StrType(0, 10), create_hdf5_dataspace(30));
    }
    {
        H5::H5File handle(path, H5F_ACC_RDONLY);
        takane::validate_array_dimnames(handle, "dimnames", extents, {});
    }
}

static void expect_error_names(const std::string& msg, const H5::Group& handle, const std::string& name, const std::vector<std::size_t>& dimensions) {
    expect_error(
        msg,
        [&]() -> void {
            takane::validate_array_dimnames(handle, name, dimensions, {});
        }
    );
}

TEST(ValidateArrayDimnames, Error) {
    auto dir = define_test_path("utils_array");
    initialize_directory(dir);
    auto path = dir / "payload.h5";
    std::vector<std::size_t> extents{ 10, 4, 30 };

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup("dimnames");
        std::vector<hsize_t> dims{ 5, 2 };
        ghandle.createDataSet("0", H5::StrType(0, 2), H5::DataSpace(2, dims.data()));
    }
    {
        H5::H5File handle(path, H5F_ACC_RDONLY);
        expect_error_names("1-dimensional", handle, "dimnames", extents);
    }

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup("dimnames");
        ghandle.createDataSet("1", H5::PredType::NATIVE_INT, create_hdf5_dataspace(4));
    }
    {
        H5::H5File handle(path, H5F_ACC_RDONLY);
        expect_error_names("UTF-8 encoded strings", handle, "dimnames", extents);
    }

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup("dimnames");
        ghandle.createDataSet("2", H5::StrType(0, 2), create_hdf5_dataspace(20));
    }
    {
        H5::H5File handle(path, H5F_ACC_RDONLY);
        expect_error_names("same length", handle, "dimnames", extents);
    }

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup("dimnames");
        ghandle.createDataSet("0", H5::StrType(0, 20), create_hdf5_dataspace(10));
        ghandle.createDataSet("foobar", H5::PredType::NATIVE_INT, create_hdf5_dataspace(30));
    }
    {
        H5::H5File handle(path, H5F_ACC_RDONLY);
        expect_error_names("more objects", handle, "dimnames", extents);
    }
}
