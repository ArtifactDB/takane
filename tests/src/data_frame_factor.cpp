#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "H5Cpp.h"

#include "utils.h"
#include "mock_data_frame.h"
#include "mock_simple_list.h"

#include <string>
#include <filesystem>
#include <fstream>

static H5::DataSet inject_codes(H5::Group& ghandle, const std::vector<int>& codes) {
    auto chandle = add_hdf5_dataset(ghandle, "codes", H5::PredType::NATIVE_UINT64, codes.size());
    chandle.write(codes.data(), H5::PredType::NATIVE_INT);
    return chandle;
}

TEST(DataFrameFactor, Okay) {
    auto dir = define_test_path("data_frame_factor");

    {
        initialize_directory_simple(dir, "data_frame_factor", "1.0");
        H5::H5File handle(dir / "contents.h5", H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup("data_frame_factor");
        inject_codes(ghandle, { 1, 3, 2, 0, 4, 0, 4, 4, 2, 3 });
        std::vector<DataFrameColumnDetails> columns(2);
        columns[0].name = "foo";
        columns[1].name = "bar";
        mock_data_frame(dir / "levels", 5, columns); 
    }
    {
        test_validate(dir);
        EXPECT_EQ(test_height(dir), 10);

        // Still works when we enable a custom duplicate levels detector.
        takane::Options opts;
        opts.data_frame_factor_any_duplicated = [](const std::filesystem::path&, const takane::ObjectMetadata&, const takane::Options&) -> bool { return false; };
        test_validate(dir, opts);
    }

    // Validates with some names.
    {
        H5::H5File handle(dir / "contents.h5", H5F_ACC_RDWR);
        auto ghandle = handle.openGroup("data_frame_factor");
        add_hdf5_dataset(ghandle, "names", H5::StrType(0, 5), 10);
    }
    {
        test_validate(dir);
        EXPECT_EQ(test_height(dir), 10);
    }
}

TEST(DataFrameFactor, LevelsError) {
    auto dir = define_test_path("data_frame_factor");

    {
        initialize_directory_simple(dir, "data_frame_factor", "1.0");
        H5::H5File handle(dir / "contents.h5", H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup("data_frame_factor");
        inject_codes(ghandle, { 1, 3, 2, 0, 4, 0, 4, 4, 2, 3 });
        initialize_directory_simple(dir / "levels", "simple_list", "1.0");
    }
    expect_validation_error(dir, "satisfies the 'DATA_FRAME' interface");

    // Check that the underlying data frame is actually validated.
    {
        initialize_directory_simple(dir, "data_frame_factor", "1.0");
        H5::H5File handle(dir / "contents.h5", H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup("data_frame_factor");
        inject_codes(ghandle, { 1, 3, 2, 0, 4, 0, 4, 4, 2, 3 });
        std::vector<DataFrameColumnDetails> columns(1);
        mock_data_frame(dir / "levels", 5, columns);
    }
    expect_validation_error(dir, "empty strings");

    // Check that the custom uniqueness function runs.
    {
        initialize_directory_simple(dir, "data_frame_factor", "1.0");
        H5::H5File handle(dir / "contents.h5", H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup("data_frame_factor");
        inject_codes(ghandle, { 1, 3, 2, 0, 4, 0, 4, 4, 2, 3 });
        std::vector<DataFrameColumnDetails> columns(1);
        columns[0].name = "foobar";
        mock_data_frame(dir / "levels", 5, {});
    }
    {
        takane::Options opts;
        opts.data_frame_factor_any_duplicated = [](const std::filesystem::path&, const takane::ObjectMetadata&, const takane::Options&) -> bool { return true; };
        expect_error(
            "should not contain duplicated rows",
            [&]() -> void {
                test_validate(dir, opts);
            }
        );
    }
}

TEST(DataFrameFactor, CodesError) {
    auto dir = define_test_path("data_frame_factor");

    // Check that validate_factor_codes() is called.
    {
        initialize_directory_simple(dir, "data_frame_factor", "1.0");
        H5::H5File handle(dir / "contents.h5", H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup("data_frame_factor");
        inject_codes(ghandle, { 1, 3, 2, 0, 4, 0, 4, 4, 2, 3 });
        mock_data_frame(dir / "levels", 4, {});
    }
    expect_validation_error(dir, "less than the number of levels");

    // No missing placeholder is respected this time.
    {
        H5::H5File handle(dir / "contents.h5", H5F_ACC_RDWR);
        auto ghandle = handle.openGroup("data_frame_factor");
        auto chandle = ghandle.openDataSet("codes");
        auto ahandle = chandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_UINT64, H5S_SCALAR);
        const int placeholder = 4; 
        ahandle.write(H5::PredType::NATIVE_INT, &placeholder);
    }
    expect_validation_error(dir, "less than the number of levels");
}

TEST(DataFrameFactor, VersionError) {
    auto dir = define_test_path("data_frame_factor");

    {
        initialize_directory_simple(dir, "data_frame_factor", "2.0");
    }
    expect_validation_error(dir, "unsupported version string");
}

TEST(DataFrameFactor, NamesError) {
    auto dir = define_test_path("data_frame_factor");

    // Check that the names are actually validated.
    {
        initialize_directory_simple(dir, "data_frame_factor", "1.0");
        H5::H5File handle(dir / "contents.h5", H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup("data_frame_factor");
        inject_codes(ghandle, { 1, 3, 2, 0, 4, 0, 4, 4, 2, 3 });
        mock_data_frame(dir / "levels", 5, {});
        add_hdf5_dataset(ghandle, "names", H5::StrType(0, 10), 20);
    }

    expect_validation_error(dir, "length of 'names'");
}

TEST(DataFrameFactor, Mcols) {
    auto dir = define_test_path("data_frame_factor");

    {
        initialize_directory_simple(dir, "data_frame_factor", "1.0");
        H5::H5File handle(dir / "contents.h5", H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup("data_frame_factor");
        inject_codes(ghandle, { 1, 3, 2, 0, 4, 5, 0, 4, 4, 2, 3, 1, 5 });

        std::vector<DataFrameColumnDetails> columns(3);
        columns[0].name = "foo";
        columns[1].name = "bar";
        columns[2].name = "whee";
        mock_data_frame(dir / "levels", 6, columns); 

        std::vector<DataFrameColumnDetails> metacolumns(2);
        metacolumns[0].name = "stuff";
        metacolumns[1].name = "boo";
        mock_data_frame(dir / "element_annotations", 13, metacolumns);
    }
    {
        test_validate(dir);
        EXPECT_EQ(test_height(dir), 13);
    }

    // Check that the mcols are properly validated.
    {
        std::filesystem::remove_all(dir / "element_annotations");
        mock_data_frame(dir / "element_annotations", 10, {});
    }
    expect_validation_error(dir, "unexpected number of rows");
}

TEST(DataFrameFactor, Metadata) {
    auto dir = define_test_path("data_frame_factor");

    {
        initialize_directory_simple(dir, "data_frame_factor", "1.0");
        H5::H5File handle(dir / "contents.h5", H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup("data_frame_factor");
        inject_codes(ghandle, { 1, 3, 2, 0, 4, 4, 2, 3, 1, 0 });

        std::vector<DataFrameColumnDetails> columns(2);
        columns[0].name = "foo";
        columns[1].name = "bar";
        mock_data_frame(dir / "levels", 5, columns); 

        mock_simple_list(dir / "other_annotations");
    }
    {
        test_validate(dir);
        EXPECT_EQ(test_height(dir), 10);
    }

    // Check that the mcols are properly validated.
    {
        std::filesystem::remove_all(dir / "other_annotations");
        mock_data_frame(dir / "other_annotations", 10, {});
    }
    expect_validation_error(dir, "SIMPLE_LIST");
}
