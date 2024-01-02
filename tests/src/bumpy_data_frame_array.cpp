#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "utils.h"
#include "data_frame.h"
#include "simple_list.h"

#include <string>
#include <filesystem>
#include <fstream>

struct BumpyDataFrameArrayTest : public::testing::Test {
    BumpyDataFrameArrayTest() {
        dir = "TEST_bumpy_data_frame_array";
        name = "bumpy_data_frame_array";
    }

    std::filesystem::path dir;
    std::string name;

    H5::H5File initialize() {
        initialize_directory_simple(dir, name, "1.0");
        return H5::H5File(dir / "partitions.h5", H5F_ACC_TRUNC);
    }

    H5::H5File reopen() {
        return H5::H5File(dir / "partitions.h5", H5F_ACC_RDWR);
    }

    void expect_error(const std::string& msg) {
        EXPECT_ANY_THROW({
            try {
                test_validate(dir);
            } catch (std::exception& e) {
                EXPECT_THAT(e.what(), ::testing::HasSubstr(msg));
                throw;
            }
        });
    }
};

TEST_F(BumpyDataFrameArrayTest, Basic) {
    initialize_directory_simple(dir, name, "2.0");
    expect_error("unsupported version string");

    {
        auto handle = initialize();
        auto ghandle = handle.createGroup(name);
        hdf5_utils::spawn_numeric_data<int>(ghandle, "lengths", H5::PredType::NATIVE_UINT32, { 4, 3, 2, 1 });
        hdf5_utils::spawn_numeric_data<int>(ghandle, "dimensions", H5::PredType::NATIVE_UINT32, { 4, 1 });
        initialize_directory_simple(dir / "concatenated", "foobar", "0.0");
    }
    expect_error("satisfy the 'DATA_FRAME'");

    initialize_directory_simple(dir / "concatenated", "data_frame", "1.0");
    expect_error("failed to validate the 'concatenated'");

    {
        data_frame::mock(dir / "concatenated", 7, {});
    }
    expect_error("sum of 'lengths'");

    {
        data_frame::mock(dir / "concatenated", 10, {});
    }
    test_validate(dir);
    EXPECT_EQ(test_height(dir), 4);
    auto dims = test_dimensions(dir);
    std::vector<size_t> expected { 4, 1 };
    EXPECT_EQ(dims, expected);
}
