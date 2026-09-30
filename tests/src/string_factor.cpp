#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "utils.h"

#include "H5Cpp.h"

#include <string>
#include <filesystem>
#include <fstream>

static H5::DataSet inject_codes(H5::Group& ghandle, const std::vector<int>& codes) {
    auto chandle = add_hdf5_dataset(ghandle, "codes", H5::PredType::NATIVE_UINT32, codes.size());
    chandle.write(codes.data(), H5::PredType::NATIVE_INT);
    return chandle;
}

static H5::DataSet inject_levels(H5::Group& ghandle, const std::vector<std::string>& levels) {
    auto lhandle = add_hdf5_dataset(ghandle, "levels", H5::StrType(0, H5T_VARIABLE), levels.size());
    auto ptrs = pointerize_strings(levels);
    lhandle.write(ptrs.data(), H5::StrType(0, H5T_VARIABLE));
    return lhandle;
}

// Most of the heavy lifting is done by the functions in utils_factor.hpp,
// so correspondingly the bulk of the tests are performed in utils_factor.cpp. 

TEST(StringFactor, Okay) {
    auto dir = define_test_path("string_factor");

    {
        initialize_directory_simple(dir, "string_factor", "1.0");
        H5::H5File handle(dir / "contents.h5", H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup("string_factor");
        inject_codes(ghandle, { 0, 3, 2, 1, 3, 0, 2 });
        inject_levels(ghandle, { "AB", "CDE", "FGHI", "JKLMNO" });
    }
    {
        test_validate(dir);
        EXPECT_EQ(test_height(dir), 7);
    }

    {
        H5::H5File handle(dir / "contents.h5", H5F_ACC_RDWR);
        auto ghandle = handle.openGroup("string_factor");
        ghandle.createAttribute("ordered", H5::PredType::NATIVE_UINT8, H5S_SCALAR);
    }
    {
        test_validate(dir);
        EXPECT_EQ(test_height(dir), 7);
    }

    {
        H5::H5File handle(dir / "contents.h5", H5F_ACC_RDWR);
        auto ghandle = handle.openGroup("string_factor");
        add_hdf5_dataset(ghandle, "names", H5::StrType(0, 10), 7);
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

    // Check that we call check_factor_ordered_attribute().
    {
        initialize_directory_simple(dir, "string_factor", "1.0");
        H5::H5File handle(dir / "contents.h5", H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup("string_factor");
        inject_codes(ghandle, { 0, 3, 2, 1, 3, 0, 2 });
        inject_levels(ghandle, { "AB", "CDE", "FGHI", "JKLMNO" });
        add_hdf5_attribute(ghandle, "ordered", "whee");
    }

    expect_validation_error(dir, "32-bit signed integer");
}

TEST(StringFactor, LevelsError) {
    auto dir = define_test_path("string_factor");

    // Check that we call validate_factor_levels().
    {
        initialize_directory_simple(dir, "string_factor", "1.0");
        H5::H5File handle(dir / "contents.h5", H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup("string_factor");
        inject_codes(ghandle, { 0, 3, 2, 1, 3, 0, 2 });
        inject_levels(ghandle, { "AB", "CDE", "FGHI", "AB" });
    }

    expect_validation_error(dir, "duplicated factor level");
}

TEST(StringFactor, CodesError) {
    auto dir = define_test_path("string_factor");

    // Check that we call validate_factor_codes().
    {
        initialize_directory_simple(dir, "string_factor", "1.0");
        H5::H5File handle(dir / "contents.h5", H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup("string_factor");
        inject_codes(ghandle, { 0, 3, 2, 1, 3, 0, 2, 3 });
        inject_levels(ghandle, { "chiyo", "ayumu", "koyomi" });
    }
    expect_validation_error(dir, "less than the number of levels");

    // Confirm that the missing placeholder is respected, which avoids this error entirely.
    {
        H5::H5File handle(dir / "contents.h5", H5F_ACC_RDWR);
        auto ghandle = handle.openGroup("string_factor");
        auto chandle = ghandle.openDataSet("codes");
        auto ahandle = chandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_UINT32, H5S_SCALAR);
        const int missing = 3;
        ahandle.write(H5::PredType::NATIVE_INT, &missing);
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
        initialize_directory_simple(dir, "string_factor", "1.0");
        H5::H5File handle(dir / "contents.h5", H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup("string_factor");
        inject_codes(ghandle, { 1, 2, 0, 3, 0, 3, 2, 1, 3, 0, 2 });
        inject_levels(ghandle, { "AB", "CDE", "FGHI", "JKLMNO" });
        add_hdf5_dataset(ghandle, "names", H5::StrType(0, 10), 7);
    }

    expect_validation_error(dir, "same length");
}
