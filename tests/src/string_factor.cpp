#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "utils.h"

#include "H5Cpp.h"

#include <string>
#include <filesystem>
#include <fstream>

static H5::Group mock_string_factor(const std::filesystem::path& dir, const std::vector<int>& codes, const std::vector<std::string>& levels) {
    initialize_directory_simple(dir, "string_factor", "1.0");
    H5::H5File handle(dir / "contents.h5", H5F_ACC_TRUNC);
    auto ghandle = handle.createGroup("string_factor");
    add_hdf5_numeric_dataset(ghandle, "codes", H5::PredType::NATIVE_UINT32, codes);
    add_hdf5_string_dataset(ghandle, "levels", levels);
    return ghandle;
}

// Most of the heavy lifting is done by the functions in utils_factor.hpp,
// so correspondingly the bulk of the tests are performed in utils_factor.cpp. 

TEST(StringFactor, Okay) {
    auto dir = define_test_path("string_factor");

    {
        mock_string_factor(
            dir,
            { 0, 3, 2, 1, 3, 0, 2 }, 
            { "AB", "CDE", "FGHI", "JKLMNO" }
        );
    }
    {
        test_validate(dir);
        EXPECT_EQ(test_height(dir), 7);
    }

    // Ordered.
    {
        auto ghandle = mock_string_factor(
            dir,
            { 0, 3, 2, 1, 3, 0, 2 }, 
            { "AB", "CDE", "FGHI", "JKLMNO" }
        );
        ghandle.createAttribute("ordered", H5::PredType::NATIVE_UINT8, H5S_SCALAR);
    }
    {
        test_validate(dir);
        EXPECT_EQ(test_height(dir), 7);
    }

    // With names.
    {
        auto ghandle = mock_string_factor(
            dir,
            { 0, 3, 2, 1, 3, 0, 2 }, 
            { "AB", "CDE", "FGHI", "JKLMNO" }
        );
        ghandle.createDataSet("names", H5::StrType(0, 10), create_hdf5_dataspace(7));
    }
    {
        test_validate(dir);
        EXPECT_EQ(test_height(dir), 7);
    }
}

TEST(StringFactor, VersionError) {
    auto dir = define_test_path("string_factor");

    {
        initialize_directory_simple(dir, "string_factor", "2.0");
    }
    expect_validation_error(dir, "unsupported version string");
}

TEST(StringFactor, OrderedError) {
    auto dir = define_test_path("string_factor");

    // Check that we actually validate the ordered attribute.
    {
        auto ghandle = mock_string_factor(
            dir,
            { 0, 3, 2, 1, 3, 0, 2 },
            { "AB", "CDE", "FGHI", "JKLMNO" }
        );
        add_hdf5_string_attribute(ghandle, "ordered", "whee");
    }

    expect_validation_error(dir, "32-bit signed integer");
}

TEST(StringFactor, LevelsError) {
    auto dir = define_test_path("string_factor");

    // Check that we call validate_factor_levels().
    {
        mock_string_factor(
            dir,
            { 0, 3, 2, 1, 3, 0, 2 },
            { "AB", "CDE", "FGHI", "AB" }
        );
    }

    expect_validation_error(dir, "duplicated factor level");
}

TEST(StringFactor, CodesError) {
    auto dir = define_test_path("string_factor");

    // Check that we call validate_factor_codes().
    {
        mock_string_factor(
            dir,
            { 0, 3, 2, 1, 3, 0, 2, 3 },
            { "chiyo", "ayumu", "koyomi" }
        );
    }
    expect_validation_error(dir, "less than the number of levels");

    // Confirm that the missing placeholder is respected, which avoids this error entirely.
    {
        auto ghandle = mock_string_factor(
            dir,
            { 0, 3, 2, 1, 3, 0, 2, 3 },
            { "chiyo", "ayumu", "koyomi" }
        );
        auto chandle = ghandle.openDataSet("codes");
        add_hdf5_numeric_attribute(chandle, "missing-value-placeholder", H5::PredType::NATIVE_UINT32, 3);
    }
    {
        test_validate(dir);
        EXPECT_EQ(test_height(dir), 8);
    }
}

TEST(StringFactor, NamesError) {
    auto dir = define_test_path("string_factor");

    // Check that we call validate_factor_levels().
    {
        auto ghandle = mock_string_factor(
            dir,
            { 1, 2, 0, 3, 0, 3, 2, 1, 3, 0, 2 },
            { "AB", "CDE", "FGHI", "JKLMNO" }
        );
        ghandle.createDataSet("names", H5::StrType(0, 10), create_hdf5_dataspace(7));
    }

    expect_validation_error(dir, "number of names");
}
