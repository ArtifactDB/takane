#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "utils.h"
#include "mock_data_frame.h"

#include <cstddef>
#include <vector>

TEST(BumpyDataFrameArray, Okay) {
    auto dir = define_test_path("bumpy_data_frame_array");

    {
        initialize_directory_simple(dir, "bumpy_data_frame_array", "1.0");
        H5::H5File handle(dir / "partitions.h5", H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup("bumpy_data_frame_array");
        add_hdf5_numeric_dataset<int>(ghandle, "lengths", H5::PredType::NATIVE_UINT32, { 0, 4, 3, 0, 2, 1 });
        add_hdf5_numeric_dataset<int>(ghandle, "dimensions", H5::PredType::NATIVE_UINT32, { 3, 2 });
        mock_data_frame(dir / "concatenated", DataFrameOptions(10, {}));
    }

    test_validate(dir);
    EXPECT_EQ(test_height(dir), 3);
    EXPECT_EQ(test_dimensions(dir), (std::vector<std::size_t>{ 3, 2 }));
}

TEST(BumpyDataFrameArray, Error) {
    auto dir = define_test_path("bumpy_data_frame_array");

    {
        initialize_directory_simple(dir, "bumpy_data_frame_array", "1.0");
        H5::H5File handle(dir / "partitions.h5", H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup("bumpy_data_frame_array");
        add_hdf5_numeric_dataset<int>(ghandle, "lengths", H5::PredType::NATIVE_UINT32, { 0, 4, 3, 0, 2, 1 });
        add_hdf5_numeric_dataset<int>(ghandle, "dimensions", H5::PredType::NATIVE_UINT32, { 3, 2 });
        mock_data_frame(dir / "concatenated", DataFrameOptions(11, {}));
    }

    expect_validation_error(dir, "sum of lengths");
}
