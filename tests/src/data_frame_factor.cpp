#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "takane/data_frame_factor.hpp"

#include "utils.h"
#include "data_frame.h"
#include "simple_list.h"

#include <string>
#include <filesystem>
#include <fstream>

struct DataFrameFactorTest : public::testing::Test {
    DataFrameFactorTest() {
        dir = "TEST_data_frame_factor";
        name = "data_frame_factor";
    }

    std::filesystem::path dir;
    std::string name;

    H5::H5File initialize() {
        initialize_directory_simple(dir, name, "1.0");
        return H5::H5File(dir / "contents.h5", H5F_ACC_TRUNC);
    }

    H5::H5File reopen() {
        auto path = dir / "contents.h5";
        return H5::H5File(path, H5F_ACC_RDWR);
    }

    template<typename ... Args_>
    void expect_error(const std::string& msg, Args_&& ... args) {
        expect_validation_error(dir, msg, std::forward<Args_>(args)...);
    }
};

TEST_F(DataFrameFactorTest, Basic) {
    initialize_directory_simple(dir, name, "2.0");
    expect_error("unsupported version string");

    // Success.
    {
        auto handle = initialize();
        auto ghandle = handle.createGroup(name);
        hdf5_utils::spawn_data(ghandle, "codes", 100, H5::PredType::NATIVE_UINT32);

        auto ldir = dir / "levels";
        data_frame::mock(ldir, 5, {});
    }
    test_validate(dir);
    EXPECT_EQ(test_height(dir), 100);
}

TEST_F(DataFrameFactorTest, Levels) {
    initialize();
    auto ldir = dir / "levels";
    initialize_directory_simple(ldir, "simple_list", "1.0");
    expect_error("'DATA_FRAME'");

    initialize_directory_simple(ldir, "data_frame", "1.0");
    expect_error("failed to validate 'levels'");

    takane::data_frame_factor::any_duplicated = [](const std::filesystem::path&, const takane::ObjectMetadata&, const takane::Options&) -> bool { return true; };
    data_frame::mock(ldir, 5, {});
    expect_error("duplicated rows");

    takane::data_frame_factor::any_duplicated = nullptr;
}

TEST_F(DataFrameFactorTest, Codes) {
    {
        auto handle = initialize();
        auto ghandle = handle.createGroup(name);

        auto ldir = dir / "levels";
        data_frame::mock(ldir, 5, {});
    }
    expect_error("codes");

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        std::vector<int> codes { 0, 4, 2, 1, 3, 5, 2 };
        auto dhandle = hdf5_utils::spawn_data(ghandle, "codes", codes.size(), H5::PredType::NATIVE_UINT32);
        dhandle.write(codes.data(), H5::PredType::NATIVE_INT);
    }
    expect_error("number of levels");

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        auto dhandle = ghandle.openDataSet("codes");
        std::vector<int> codes { 0, 1, 2, 1, 3, 4, 2 };
        dhandle.write(codes.data(), H5::PredType::NATIVE_INT);
    }
    test_validate(dir);
}

TEST_F(DataFrameFactorTest, Names) {
    std::vector<int> codes { 0, 1, 2, 1, 0, 1, 2 };
    {
        auto handle = initialize();
        auto ghandle = handle.createGroup(name);

        auto dhandle = hdf5_utils::spawn_data(ghandle, "codes", codes.size(), H5::PredType::NATIVE_UINT32);
        dhandle.write(codes.data(), H5::PredType::NATIVE_INT);
        hdf5_utils::spawn_data(ghandle, "names", codes.size(), H5::PredType::NATIVE_INT);

        auto ldir = dir / "levels";
        data_frame::mock(ldir, 5, {});
    }
    expect_error("string datatype");

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        ghandle.unlink("names");
        hdf5_utils::spawn_data(ghandle, "names", 50, H5::StrType(0, 10));
    }
    expect_error("same length");

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        ghandle.unlink("names");
        hdf5_utils::spawn_data(ghandle, "names", codes.size(), H5::StrType(0, 10));
    }
    test_validate(dir);
}

TEST_F(DataFrameFactorTest, Metadata) {
    auto edir = dir / "element_annotations";
    auto odir = dir / "other_annotations";

    std::vector<int> codes { 0, 1, 2, 1, 3, 1, 0, 2 };
    {
        auto handle = initialize();
        auto ghandle = handle.createGroup(name);

        auto dhandle = hdf5_utils::spawn_data(ghandle, "codes", codes.size(), H5::PredType::NATIVE_UINT32);
        dhandle.write(codes.data(), H5::PredType::NATIVE_INT);

        auto ldir = dir / "levels";
        data_frame::mock(ldir, 5, {});

        initialize_directory_simple(edir, "simple_list", "1.0");
    }
    expect_error("'element_annotations'");

    data_frame::mock(edir, codes.size(), {});
    initialize_directory_simple(odir, "data_frame", "1.0");
    expect_error("'other_annotations'");

    simple_list::mock(odir);
    test_validate(dir);
}
