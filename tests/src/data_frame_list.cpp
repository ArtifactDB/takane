#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "takane/takane.hpp"
#include "utils.h"
#include "data_frame.h"
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

    template<typename ... Args_>
    void expect_error(const std::string& msg, Args_&& ... args) {
        EXPECT_ANY_THROW({
            try {
                takane::validate(dir, std::forward<Args_>(args)...);
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
        data_frame::mock(dir / "concatenated", 7, false, {});
    }
    expect_error("sum of 'lengths'");

    {
        data_frame::mock(dir / "concatenated", 10, false, {});
    }
    takane::validate(dir);
    EXPECT_EQ(takane::height(dir), 4);
}

TEST_F(DataFrameListTest, Names) {
    {
        initialize_directory(dir, name);
        auto handle = H5::H5File(dir / "partitions.h5", H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup(name);
        hdf5_utils::attach_attribute(ghandle, "version", "1.0");
        hdf5_utils::spawn_numeric_data<int>(ghandle, "lengths", H5::PredType::NATIVE_UINT32, { 4, 3, 2, 1 });
        data_frame::mock(dir / "concatenated", 10, false, {});

        hdf5_utils::spawn_string_data(ghandle, "names", H5T_VARIABLE, { "Aaron", "Charlie", "Echo", "Fooblewooble" });
    }
    takane::validate(dir);

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        ghandle.unlink("names");
        hdf5_utils::spawn_string_data(ghandle, "names", H5T_VARIABLE, { "Aaron" });
    }
    expect_error("same length");
}

TEST_F(DataFrameListTest, Metadata) {
    {
        initialize_directory(dir, name);
        auto handle = H5::H5File(dir / "partitions.h5", H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup(name);
        hdf5_utils::attach_attribute(ghandle, "version", "1.0");
        hdf5_utils::spawn_numeric_data<int>(ghandle, "lengths", H5::PredType::NATIVE_UINT32, { 4, 3, 2, 1 });
        data_frame::mock(dir / "concatenated", 10, false, {});
    }

    auto cdir = dir / "element_annotations";
    auto odir = dir / "other_annotations";

    initialize_directory(cdir, "simple_list");
    expect_error("'DATA_FRAME'"); 

    data_frame::mock(cdir, 4, false, {});
    initialize_directory(odir, "data_frame");
    expect_error("'SIMPLE_LIST'");

    initialize_directory(odir, "simple_list");
    simple_list::mock(odir);
    takane::validate(dir);
}
