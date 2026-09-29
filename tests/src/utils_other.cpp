#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "takane/utils_other.hpp"

#include "mock_data_frame.h"
#include "mock_simple_list.h"
#include "utils.h"

TEST(ValidateMcols, Okay) {
    auto dir = define_test_path("utils_other");
    initialize_directory(dir);

    // No-op if the directory doesn't exist.
    takane::Options opts;
    std::string name = "mcols";
    takane::validate_mcols(dir, name, 10, opts);

    // Alright, actually creating the directory.
    {
        mock_data_frame(dir / name, 10, {});
    }
    takane::validate_mcols(dir, name, 10, opts);
}

TEST(ValidateMcols, Error) {
    auto dir = define_test_path("utils_other");
    initialize_directory(dir);

    takane::Options opts;
    std::string name = "mcols";
    mock_data_frame(dir / name, 10, {});

    expect_error(
        "unexpected number of rows",
        [&]() -> void {
            takane::validate_mcols(dir, name, 11, opts);
        }
    );

    // Check that we actually validate the data_frame.
    {
        H5::H5File handle(dir / name / "basic_columns.h5", H5F_ACC_RDWR);
        auto ghandle = handle.openGroup("data_frame");
        auto dhandle = ghandle.openGroup("data");
        dhandle.createGroup("0");
    }
    expect_error(
        "more objects",
        [&]() -> void {
            takane::validate_mcols(dir, name, 10, opts);
        }
    );

    initialize_directory_simple(dir / name, "simple_list", "1.0");
    expect_error(
        "DATA_FRAME",
        [&]() -> void {
            takane::validate_mcols(dir, name, 10, opts);
        }
    );
}

TEST(ValidateMetadata, Okay) {
    auto dir = define_test_path("utils_other");
    initialize_directory(dir);

    // No-op if the directory doesn't exist.
    takane::Options opts;
    std::string name = "metadata";
    takane::validate_metadata(dir, name, opts);

    {
        mock_simple_list(dir / name);
    }
    takane::validate_metadata(dir, "metadata", opts);
}

TEST(ValidateMetadata, Error) {
    auto dir = define_test_path("utils_other");
    initialize_directory(dir);

    takane::Options opts;
    std::string name = "metadata";

    // Check that we actually validate the simple_list.
    {
        initialize_simple_list_with_metadata(dir / name, "1.0", "json.gz");
        dump_compressed_json(dir / name, "{ \"type\": \"integer\", \"values\": [] }");
    }
    expect_error(
        "top-level",
        [&]() -> void {
            takane::validate_metadata(dir, name, opts);
        }
    );

    {
        mock_data_frame(dir / name, 10, {});
    }
    expect_error(
        "SIMPLE_LIST",
        [&]() -> void {
            takane::validate_metadata(dir, name, opts);
        }
    );
}

TEST(CountDirectoryEntries, Basic) {
    auto dir = define_test_path("utils_other");
    auto path = dir / "counts";
    initialize_directory(path);

    EXPECT_EQ(takane::count_directory_entries(path), 0);

    std::filesystem::create_directory(path / "blah");
    std::filesystem::create_directory(path / "asdasd");
    EXPECT_EQ(takane::count_directory_entries(path), 2);

    // Ignores . and _ prefixes.
    std::filesystem::create_directory(path / "_whee");
    std::filesystem::create_directory(path / ".foo");
    EXPECT_EQ(takane::count_directory_entries(path), 2);
}
