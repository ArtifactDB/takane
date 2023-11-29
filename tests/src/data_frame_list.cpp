#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "takane/takane.hpp"
#include "utils.h"
#include "data_frame.h"
#include "simple_list.h"

#include <string>
#include <filesystem>
#include <fstream>

struct DataFrameListTest : public::testing::Test {
    DataFrameListTest() {
        dir = "TEST_data_frame_list";
        name = "data_frame_list";
    }

    std::filesystem::path dir;
    std::string name;

    H5::H5File initialize() {
        initialize_directory(dir, name);
        return H5::H5File(dir / "partitions.h5", H5F_ACC_TRUNC);
    }

    H5::H5File reopen() {
        return H5::H5File(dir / "partitions.h5", H5F_ACC_RDWR);
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

TEST_F(DataFrameListTest, Basic) {
    {
        auto handle = initialize();
        auto ghandle = handle.createGroup(name);
        hdf5_utils::attach_attribute(ghandle, "version", "2.0");
    }
    expect_error("unsupported version string");

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        ghandle.removeAttr("version");
        hdf5_utils::attach_attribute(ghandle, "version", "1.0");
        hdf5_utils::spawn_numeric_data<int>(ghandle, "lengths", H5::PredType::NATIVE_UINT32, { 4, 3, 2, 1 });
        initialize_directory(dir / "concatenated", "foobar");
    }
    expect_error("satisfy the 'DATA_FRAME'");

    {
        initialize_directory(dir / "concatenated", "data_frame");
    }
    expect_error("failed to validate the 'concatenated'");

    {
        data_frame::mock(dir / "concatenated", 7, {});
    }
    expect_error("sum of 'lengths'");

    {
        data_frame::mock(dir / "concatenated", 10, {});
    }
    takane::validate(dir);
    EXPECT_EQ(takane::height(dir), 4);
}