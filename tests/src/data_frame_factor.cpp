#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "takane/takane.hpp"
#include "utils.h"
#include "data_frame.h"
#include "simple_list.h"

#include <string>
#include <filesystem>
#include <fstream>

struct DataFrameFactorTest : public::testing::Test {
    static std::filesystem::path testdir() {
        return "TEST_data_frame_factor";
    }

    static H5::H5File initialize() {
        auto path = testdir();
        initialize_directory(path, "data_frame_factor");
        path.append("contents.h5");
        return H5::H5File(path, H5F_ACC_TRUNC);
    }

    static H5::H5File reopen() {
        auto path = testdir() / "contents.h5";
        return H5::H5File(path, H5F_ACC_RDWR);
    }

    template<typename ... Args_>
    static void expect_error(const std::string& msg, Args_&& ... args) {
        expect_validation_error(testdir(), msg, std::forward<Args_>(args)...);
    }
};

TEST_F(DataFrameFactorTest, Basic) {
    {
        auto handle = initialize();
        auto ghandle = handle.createGroup("data_frame_factor");
        hdf5_utils::attach_attribute(ghandle, "version", "2.0");
    }
    expect_error("unsupported version string");

    auto ldir = testdir() / "levels";
    {
        auto handle = reopen();
        auto ghandle = handle.openGroup("data_frame_factor");
        ghandle.removeAttr("version");
        hdf5_utils::attach_attribute(ghandle, "version", "1.0");
        initialize_directory(ldir, "foobar");
    }
    expect_error("'levels'");

    data_frame::mock(ldir, 5, {});
    expect_error("'codes'");

    // Success at last.
    {
        auto handle = reopen();
        auto ghandle = handle.openGroup("data_frame_factor");
        hdf5_utils::spawn_data(ghandle, "codes", 100, H5::PredType::NATIVE_UINT32);
    }
    takane::validate(testdir());
    EXPECT_EQ(takane::height(testdir()), 100);
}

TEST_F(DataFrameFactorTest, Levels) {
    {
        auto handle = initialize();
        auto ghandle = handle.createGroup("data_frame_factor");
        hdf5_utils::attach_attribute(ghandle, "version", "1.0");
    }

    auto ldir = testdir() / "levels";
    {
        initialize_directory(ldir, "simple_list");
    }
    expect_error("'DATA_FRAME'");

    takane::data_frame_factor::any_duplicated = [](const std::filesystem::path&, const takane::ObjectMetadata&, const takane::Options&) -> bool { return true; };
    data_frame::mock(ldir, 5, {});
    expect_error("duplicated rows");

    takane::data_frame_factor::any_duplicated = nullptr;
}

TEST_F(DataFrameFactorTest, Codes) {
    {
        auto handle = initialize();
        auto ghandle = handle.createGroup("data_frame_factor");
        hdf5_utils::attach_attribute(ghandle, "version", "1.0");

        std::vector<int> codes { 0, 4, 2, 1, 3, 5, 2 };
        auto dhandle = hdf5_utils::spawn_data(ghandle, "codes", codes.size(), H5::PredType::NATIVE_UINT32);
        dhandle.write(codes.data(), H5::PredType::NATIVE_INT);

        auto ldir = testdir() / "levels";
        data_frame::mock(ldir, 5, {});
    }
    expect_error("number of levels");

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup("data_frame_factor");
        auto dhandle = ghandle.openDataSet("codes");
        std::vector<int> codes { 0, 1, 2, 1, 3, 4, 2 };
        dhandle.write(codes.data(), H5::PredType::NATIVE_INT);
    }
    takane::validate(testdir());
}

TEST_F(DataFrameFactorTest, Names) {
    std::vector<int> codes { 0, 1, 2, 1, 0, 1, 2 };
    {
        auto handle = initialize();
        auto ghandle = handle.createGroup("data_frame_factor");
        hdf5_utils::attach_attribute(ghandle, "version", "1.0");

        auto dhandle = hdf5_utils::spawn_data(ghandle, "codes", codes.size(), H5::PredType::NATIVE_UINT32);
        dhandle.write(codes.data(), H5::PredType::NATIVE_INT);
        hdf5_utils::spawn_data(ghandle, "names", codes.size(), H5::PredType::NATIVE_INT);

        auto ldir = testdir() / "levels";
        data_frame::mock(ldir, 5, {});
    }
    expect_error("string datatype");

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup("data_frame_factor");
        ghandle.unlink("names");
        hdf5_utils::spawn_data(ghandle, "names", 50, H5::StrType(0, 10));
    }
    expect_error("same length");

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup("data_frame_factor");
        ghandle.unlink("names");
        hdf5_utils::spawn_data(ghandle, "names", codes.size(), H5::StrType(0, 10));
    }
    takane::validate(testdir());
}

TEST_F(DataFrameFactorTest, Metadata) {
    auto dir = testdir();
    auto edir = dir / "element_annotations";
    auto odir = dir / "other_annotations";

    std::vector<int> codes { 0, 1, 2, 1, 3, 1, 0, 2 };
    {
        auto handle = initialize();
        auto ghandle = handle.createGroup("data_frame_factor");
        hdf5_utils::attach_attribute(ghandle, "version", "1.0");

        auto dhandle = hdf5_utils::spawn_data(ghandle, "codes", codes.size(), H5::PredType::NATIVE_UINT32);
        dhandle.write(codes.data(), H5::PredType::NATIVE_INT);

        auto ldir = dir / "levels";
        data_frame::mock(ldir, 5, {});

        initialize_directory(edir, "simple_list");
    }
    expect_error("'element_annotations'");

    data_frame::mock(edir, codes.size(), {});
    initialize_directory(odir, "data_frame");
    expect_error("'other_annotations'");

    simple_list::mock(odir);
    takane::validate(testdir());
}
