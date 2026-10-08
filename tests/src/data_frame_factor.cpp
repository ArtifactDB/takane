#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "H5Cpp.h"

#include "utils.h"
#include "mock_data_frame.h"
#include "mock_simple_list.h"

#include <string>
#include <filesystem>
#include <fstream>

static H5::Group mock_factor_codes(const std::filesystem::path& dir, const std::vector<int>& codes) {
    H5::H5File handle(dir / "contents.h5", H5F_ACC_TRUNC);
    auto ghandle = handle.createGroup("data_frame_factor");
    add_hdf5_numeric_dataset(ghandle, "codes", H5::PredType::NATIVE_UINT64, codes);
    return ghandle;
}

TEST(DataFrameFactor, Okay) {
    auto dir = define_test_path("data_frame_factor");

    std::vector<DataFrameColumnDetails> columns(2);
    columns[0].name = "foo";
    columns[1].name = "bar";

    {
        initialize_directory_simple(dir, "data_frame_factor", "1.0");
        mock_factor_codes(dir, { 1, 3, 2, 0, 4, 0, 4, 4, 2, 3 });
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
        initialize_directory_simple(dir, "data_frame_factor", "1.0");
        auto ghandle = mock_factor_codes(dir, { 1, 3, 2, 0, 4, 0, 4, 4, 2, 3 });
        ghandle.createDataSet("names", H5::StrType(0, 5), create_hdf5_dataspace(10));
        mock_data_frame(dir / "levels", 5, columns); 
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
        mock_factor_codes(dir, { 1, 3, 2, 0, 4, 0, 4, 4, 2, 3 });
        initialize_directory_simple(dir / "levels", "simple_list", "1.0");
    }
    expect_validation_error(dir, "satisfies the 'DATA_FRAME' interface");

    // Check that the underlying data frame is actually validated.
    {
        initialize_directory_simple(dir, "data_frame_factor", "1.0");
        mock_factor_codes(dir, { 1, 3, 2, 0, 4, 0, 4, 4, 2, 3 });
        std::vector<DataFrameColumnDetails> columns(1);
        mock_data_frame(dir / "levels", 5, columns);
    }
    expect_validation_error(dir, "empty strings");

    // Check that the custom uniqueness function runs.
    {
        initialize_directory_simple(dir, "data_frame_factor", "1.0");
        mock_factor_codes(dir, { 1, 3, 2, 0, 4, 0, 4, 4, 2, 3 });
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
        mock_factor_codes(dir, { 1, 3, 2, 0, 4, 0, 4, 4, 2, 3 });
        mock_data_frame(dir / "levels", 4, {});
    }
    expect_validation_error(dir, "less than the number of levels");

    // No missing placeholder is respected this time.
    {
        initialize_directory_simple(dir, "data_frame_factor", "1.0");
        auto ghandle = mock_factor_codes(dir, { 1, 3, 2, 0, 4, 0, 4, 4, 2, 3 });
        auto chandle = ghandle.openDataSet("codes");
        add_hdf5_numeric_attribute(chandle, "missing-value-placeholder", H5::PredType::NATIVE_UINT64, 4);
        mock_data_frame(dir / "levels", 4, {});
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
        auto ghandle = mock_factor_codes(dir, { 1, 3, 2, 0, 4, 0, 4, 4, 2, 3 });
        ghandle.createDataSet("names", H5::StrType(0, 10), create_hdf5_dataspace(20));
        mock_data_frame(dir / "levels", 5, {});
    }
    expect_validation_error(dir, "number of names");
}

TEST(DataFrameFactor, McolsOkay) {
    auto dir = define_test_path("data_frame_factor");

    {
        initialize_directory_simple(dir, "data_frame_factor", "1.0");
        mock_factor_codes(dir, { 1, 3, 2, 0, 4, 5, 0, 4, 4, 2, 3, 1, 5 });

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

    test_validate(dir);
    EXPECT_EQ(test_height(dir), 13);
}

TEST(DataFrameFactor, McolsError) {
    auto dir = define_test_path("data_frame_factor");

    // Check that the mcols are properly validated.
    {
        initialize_directory_simple(dir, "data_frame_factor", "1.0");
        mock_factor_codes(dir, { 5, 1, 2, 1, 3, 5, 0, 4, 0, 0, 0, 3 });

        std::vector<DataFrameColumnDetails> columns(2);
        columns[0].name = "stuff";
        columns[1].name = "blah";
        mock_data_frame(dir / "levels", 6, columns); 

        mock_data_frame(dir / "element_annotations", 30, {});
    }
    expect_validation_error(dir, "unexpected number of rows");
}

TEST(DataFrameFactor, MetadataOkay) {
    auto dir = define_test_path("data_frame_factor");

    {
        initialize_directory_simple(dir, "data_frame_factor", "1.0");
        mock_factor_codes(dir, { 2, 3, 2, 0, 1, 4, 4, 3, 4, 0 });

        std::vector<DataFrameColumnDetails> columns(2);
        columns[0].name = "foo";
        columns[1].name = "bar";
        mock_data_frame(dir / "levels", 5, columns); 

        mock_simple_list(dir / "other_annotations");
    }

    test_validate(dir);
    EXPECT_EQ(test_height(dir), 10);
}

TEST(DataFrameFactor, MetadataError) {
    auto dir = define_test_path("data_frame_factor");

    // Check that the mcols are properly validated.
    {
        initialize_directory_simple(dir, "data_frame_factor", "1.0");
        mock_factor_codes(dir, { 1, 0, 0, 1, 1, 0, 1 });

        std::vector<DataFrameColumnDetails> columns(2);
        columns[0].name = "foo";
        columns[1].name = "bar";
        mock_data_frame(dir / "levels", 2, columns); 

        mock_data_frame(dir / "other_annotations", 10, {});
    }
    expect_validation_error(dir, "SIMPLE_LIST");
}
