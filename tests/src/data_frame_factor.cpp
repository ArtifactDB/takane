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

    DataFrameOptions opt(5, { DataFrameColumn("foo"), DataFrameColumn("bar") });
    std::vector<int> codes { 1, 3, 2, 0, 4, 0, 4, 4, 2, 3 };

    {
        initialize_directory_simple(dir, "data_frame_factor", "1.0");
        mock_factor_codes(dir, codes);
        mock_data_frame(dir / "levels", opt);
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
        auto ghandle = mock_factor_codes(dir, codes);
        ghandle.createDataSet("names", H5::StrType(0, 5), create_hdf5_dataspace(codes.size()));
        mock_data_frame(dir / "levels", opt); 
    }
    {
        test_validate(dir);
        EXPECT_EQ(test_height(dir), 10);
    }
}

TEST(DataFrameFactor, LevelsError) {
    auto dir = define_test_path("data_frame_factor");

    std::vector<int> codes { 4, 1, 2, 2, 3, 0, 1, 2 };

    {
        initialize_directory_simple(dir, "data_frame_factor", "1.0");
        mock_factor_codes(dir, codes);
        initialize_directory_simple(dir / "levels", "simple_list", "1.0");
    }
    expect_validation_error(dir, "satisfies the 'DATA_FRAME' interface");

    // Check that the underlying data frame is actually validated.
    {
        initialize_directory_simple(dir, "data_frame_factor", "1.0");
        mock_factor_codes(dir, codes);
        mock_data_frame(dir / "levels", DataFrameOptions(5, { DataFrameColumn("") }));
    }
    expect_validation_error(dir, "empty strings");

    // Check that the custom uniqueness function runs.
    {
        initialize_directory_simple(dir, "data_frame_factor", "1.0");
        mock_factor_codes(dir, codes);
        mock_data_frame(dir / "levels", DataFrameOptions(5, { DataFrameColumn("foobar") }));
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

    std::vector<int> codes{ 1, 0, 1, 2, 3, 4, 1, 2, 1, 3, 0, 3 };

    // Check that validate_factor_codes() is called.
    {
        initialize_directory_simple(dir, "data_frame_factor", "1.0");
        mock_factor_codes(dir, codes);
        mock_data_frame(dir / "levels", DataFrameOptions(4, {}));
    }
    expect_validation_error(dir, "less than the number of levels");

    // No missing placeholder is respected this time.
    {
        initialize_directory_simple(dir, "data_frame_factor", "1.0");
        auto ghandle = mock_factor_codes(dir, codes);
        auto chandle = ghandle.openDataSet("codes");
        add_hdf5_numeric_attribute(chandle, "missing-value-placeholder", H5::PredType::NATIVE_UINT64, 4);
        mock_data_frame(dir / "levels", DataFrameOptions(4, {}));
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
        mock_data_frame(dir / "levels", DataFrameOptions(5, {}));
    }
    expect_validation_error(dir, "number of names");
}

TEST(DataFrameFactor, McolsOkay) {
    auto dir = define_test_path("data_frame_factor");

    std::vector<int> codes{ 3, 0, 1, 4, 2, 4, 5, 0, 4, 4, 2, 3, 1, 5 };

    {
        initialize_directory_simple(dir, "data_frame_factor", "1.0");
        mock_factor_codes(dir, codes);

        DataFrameOptions opt(6, { DataFrameColumn("foo"), DataFrameColumn("bar"), DataFrameColumn("whee") });
        mock_data_frame(dir / "levels", opt);

        DataFrameOptions metaopt(codes.size(), { DataFrameColumn("stuff"), DataFrameColumn("boo") });
        mock_data_frame(dir / "element_annotations", metaopt);
    }

    test_validate(dir);
    EXPECT_EQ(test_height(dir), codes.size());
}

TEST(DataFrameFactor, McolsError) {
    auto dir = define_test_path("data_frame_factor");

    // Check that the mcols are properly validated.
    {
        initialize_directory_simple(dir, "data_frame_factor", "1.0");
        mock_factor_codes(dir, { 5, 1, 2, 1, 3, 5, 0, 4, 0, 0, 0, 3 });

        DataFrameOptions opt(6, { DataFrameColumn("stuff"), DataFrameColumn("blah") });
        mock_data_frame(dir / "levels", opt);

        mock_data_frame(dir / "element_annotations", DataFrameOptions(30, {}));
    }
    expect_validation_error(dir, "unexpected number of rows");
}

TEST(DataFrameFactor, MetadataOkay) {
    auto dir = define_test_path("data_frame_factor");

    {
        initialize_directory_simple(dir, "data_frame_factor", "1.0");
        mock_factor_codes(dir, { 2, 3, 2, 0, 1, 4, 4, 3, 4, 0 });

        DataFrameOptions opt(5, { DataFrameColumn("foo"), DataFrameColumn("bar") });
        mock_data_frame(dir / "levels", opt);

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

        DataFrameOptions opt(2, { DataFrameColumn("foo"), DataFrameColumn("bar") });
        mock_data_frame(dir / "levels", opt); 

        mock_data_frame(dir / "other_annotations", DataFrameOptions(10, {}));
    }
    expect_validation_error(dir, "SIMPLE_LIST");
}
