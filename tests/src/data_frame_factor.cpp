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
        EXPECT_ANY_THROW({
            try {
                takane::validate(testdir(), std::forward<Args_>(args)...);
            } catch (std::exception& e) {
                EXPECT_THAT(e.what(), ::testing::HasSubstr(msg));
                throw;
            }
        });
    }
};

TEST_F(DataFrameFactorTest, Basic) {
    {
        auto handle = initialize();
    }
    expect_error("expected a 'data_frame_factor' group");

    {
        auto handle = reopen();
        handle.createDataSet("data_frame_factor", H5::PredType::NATIVE_INT, H5S_SCALAR);
    }
    expect_error("expected a 'data_frame_factor' group");

    {
        auto handle = reopen();
        handle.unlink("data_frame_factor");
        auto ghandle = handle.createGroup("data_frame_factor");
        hdf5_utils::attach_attribute(ghandle, "version", "2.0");
    }
    expect_error("unsupported version string");

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup("data_frame_factor");
        ghandle.removeAttr("version");
        hdf5_utils::attach_attribute(ghandle, "version", "1.0");
    }
    expect_error("'levels'");

    {
        auto ldir = testdir() / "levels";
        initialize_directory(ldir, "data_frame");
        data_frame::mock(ldir, 5, false, {});
    }
    expect_error("'codes'");

    // Success at last.
    {
        auto handle = reopen();
        auto ghandle = handle.openGroup("data_frame_factor");
        hdf5_utils::spawn_data(ghandle, "codes", 100, H5::PredType::NATIVE_INT32);
    }
    takane::validate(testdir());
    EXPECT_EQ(takane::height(testdir()), 100);
}

TEST_F(DataFrameFactorTest, CodeChecks) {
    {
        auto handle = initialize();
        auto ghandle = handle.createGroup("data_frame_factor");
        hdf5_utils::attach_attribute(ghandle, "version", "1.0");

        std::vector<int> codes { 0, -1, 2, 1, 3, -1, 2 };
        auto dhandle = hdf5_utils::spawn_data(ghandle, "codes", codes.size(), H5::PredType::NATIVE_INT32);
        dhandle.write(codes.data(), H5::PredType::NATIVE_INT);

        auto ldir = testdir() / "levels";
        initialize_directory(ldir, "data_frame");
        data_frame::mock(ldir, 5, false, {});
    }
    expect_error("non-negative");

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup("data_frame_factor");
        auto dhandle = ghandle.openDataSet("codes");
        std::vector<int> codes { 0, 1, 2, 1, 3, 100, 2 };
        dhandle.write(codes.data(), H5::PredType::NATIVE_INT);
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

TEST_F(DataFrameFactorTest, NameChecks) {
    std::vector<int> codes { 0, 1, 2, 1, 0, 1, 2 };
    {
        auto handle = initialize();
        auto ghandle = handle.createGroup("data_frame_factor");
        hdf5_utils::attach_attribute(ghandle, "version", "1.0");

        auto dhandle = hdf5_utils::spawn_data(ghandle, "codes", codes.size(), H5::PredType::NATIVE_INT32);
        dhandle.write(codes.data(), H5::PredType::NATIVE_INT);
        hdf5_utils::spawn_data(ghandle, "names", codes.size(), H5::PredType::NATIVE_INT);

        auto ldir = testdir() / "levels";
        initialize_directory(ldir, "data_frame");
        data_frame::mock(ldir, 5, false, {});
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

TEST_F(DataFrameFactorTest, MetadataChecks) {
    auto dir = testdir();
    auto edir = dir / "element_annotations";
    auto odir = dir / "other_annotations";

    std::vector<int> codes { 0, 1, 2, 1, 3, 1, 0, 2 };
    {
        auto handle = initialize();
        auto ghandle = handle.createGroup("data_frame_factor");
        hdf5_utils::attach_attribute(ghandle, "version", "1.0");

        auto dhandle = hdf5_utils::spawn_data(ghandle, "codes", codes.size(), H5::PredType::NATIVE_INT32);
        dhandle.write(codes.data(), H5::PredType::NATIVE_INT);

        auto ldir = dir / "levels";
        initialize_directory(ldir, "data_frame");
        data_frame::mock(ldir, 5, false, {});

        initialize_directory(edir, "simple_list");
    }
    expect_error("'element_annotations'");

    {
        initialize_directory(edir, "data_frame");
        data_frame::mock(edir, codes.size(), false, {});
        initialize_directory(odir, "data_frame");
    }
    expect_error("'other_annotations'");

    {
        initialize_directory(odir, "simple_list");
        simple_list::mock(odir);
    }
    takane::validate(testdir());
}
