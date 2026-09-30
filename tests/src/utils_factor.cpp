#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "takane/utils_factor.hpp"

#include "utils.h"

TEST(CheckFactorOrderedAttribute, Okay) {
    auto path = define_test_path("utils_factor");

    // No-op if it doesn't exist.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup("foo");
    }
    {
        H5::H5File handle(path, H5F_ACC_RDONLY);
        takane::check_factor_ordered_attribute(handle.openGroup("foo"));
    }

    // Passes fine if it exists. 
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup("foo");
        ghandle.createAttribute("ordered", H5::PredType::NATIVE_INT32, H5S_SCALAR);
    }
    {
        H5::H5File handle(path, H5F_ACC_RDONLY);
        takane::check_factor_ordered_attribute(handle.openGroup("foo"));
    }
}

TEST(CheckFactorOrderedAttribute, Error) {
    auto path = define_test_path("utils_factor");

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup("foo");
        constexpr hsize_t one = 1;
        ghandle.createAttribute("ordered", H5::PredType::NATIVE_INT32, H5::DataSpace(1, &one));
    }
    {
        H5::H5File handle(path, H5F_ACC_RDONLY);
        expect_error(
            "scalar",
            [&]() -> void {
                takane::check_factor_ordered_attribute(handle.openGroup("foo"));
            }
        );
    }

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup("foo");
        ghandle.createAttribute("ordered", H5::PredType::NATIVE_DOUBLE, H5S_SCALAR);
    }
    {
        H5::H5File handle(path, H5F_ACC_RDONLY);
        expect_error(
            "32-bit signed integer",
            [&]() -> void {
                takane::check_factor_ordered_attribute(handle.openGroup("foo"));
            }
        );
    }
}

/******************************************/

TEST(ValidateFactorLevels, Okay) {
    auto path = define_test_path("utils_factor");

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto dhandle = add_hdf5_dataset(handle, "foobar", H5::StrType(0, H5T_VARIABLE), 5);
        std::vector<std::string> levels { "A", "BB", "CCC", "DDDD", "EEEEE" };
        auto lptrs = pointerize_strings(levels);
        dhandle.write(lptrs.data(), H5::StrType(0, H5T_VARIABLE));
    }

    H5::H5File handle(path, H5F_ACC_RDONLY);
    auto dhandle = handle.openDataSet("foobar");
    EXPECT_EQ(takane::validate_factor_levels(dhandle, 10000), 5);
    EXPECT_EQ(takane::validate_factor_levels(dhandle, 2), 5); // works with a smaller buffer size.
}

TEST(ValidateFactorLevels, Error) {
    auto path = define_test_path("utils_factor");

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        add_hdf5_dataset(handle, "foobar", H5::PredType::NATIVE_INT32, 5);
    }
    {
        H5::H5File handle(path, H5F_ACC_RDONLY);
        auto dhandle = handle.openDataSet("foobar");
        expect_error(
            "UTF-8 encoded string",
            [&]() -> void {
                takane::validate_factor_levels(dhandle, 10000);
            }
        );
    }

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        add_hdf5_dataset(handle, "foobar", H5::StrType(0, 10), 5);
    }
    {
        H5::H5File handle(path, H5F_ACC_RDONLY);
        auto dhandle = handle.openDataSet("foobar");
        expect_error(
            "duplicated",
            [&]() -> void {
                takane::validate_factor_levels(dhandle, 10000);
            }
        );
    }

    // Less trivial example.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto dhandle = add_hdf5_dataset(handle, "foobar", H5::StrType(0, H5T_VARIABLE), 5);
        std::vector<std::string> levels { "A", "BB", "CCC", "DDDD", "A" };
        auto lptrs = pointerize_strings(levels);
        dhandle.write(lptrs.data(), H5::StrType(0, H5T_VARIABLE));
    }
    {
        H5::H5File handle(path, H5F_ACC_RDONLY);
        auto dhandle = handle.openDataSet("foobar");
        expect_error(
            "duplicated",
            [&]() -> void {
                takane::validate_factor_levels(dhandle, 2); // throws with a smaller buffer size.
            }
        );
    }
}

/******************************************/

TEST(ValidateFactorCodes, Okay) {
    auto path = define_test_path("utils_factor");
    const std::size_t nlevels = 4;

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        std::vector<int> vals { 3, 2, 0, 1, 2, 1, 1, 2, 3, 0 };
        auto dhandle = add_hdf5_dataset(handle, "foo", H5::PredType::NATIVE_UINT32, vals.size());
        dhandle.write(vals.data(), H5::PredType::NATIVE_INT);
    }
    {
        H5::H5File handle(path, H5F_ACC_RDONLY);
        auto dhandle = handle.openDataSet("foo");
        takane::validate_factor_codes(dhandle, nlevels, 1000, true);
        takane::validate_factor_codes(dhandle, nlevels, 1000, false); // check for a missing placeholder.
        takane::validate_factor_codes(dhandle, nlevels, 3, true); // trying with a smaller buffer size.
        takane::validate_factor_codes(dhandle, nlevels, 3, false);
    }

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        std::vector<int> vals { 3, 2, 0, 100, 2, 1, 1, 100, 3, 0 };
        auto dhandle = add_hdf5_dataset(handle, "foo", H5::PredType::NATIVE_UINT32, vals.size());
        dhandle.write(vals.data(), H5::PredType::NATIVE_INT);
        auto ahandle = dhandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_UINT32, H5S_SCALAR);
        int val = 100;
        ahandle.write(H5::PredType::NATIVE_INT, &val);
    }
    {
        H5::H5File handle(path, H5F_ACC_RDONLY);
        takane::validate_factor_codes(handle.openDataSet("foo"), nlevels, 1000, true);
        takane::validate_factor_codes(handle.openDataSet("foo"), nlevels, 3, true); // trying with a smaller buffer size.
    }
}

TEST(ValidateFactorCodes, GeneralError) {
    auto path = define_test_path("utils_factor");
    const std::size_t nlevels = 4;

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        add_hdf5_dataset(handle, "foo", H5::PredType::NATIVE_DOUBLE, 10);
    }
    {
        H5::H5File handle(path, H5F_ACC_RDONLY);
        auto dhandle = handle.openDataSet("foo");
        expect_error(
            "64-bit unsigned integer",
            [&]() -> void {
                takane::validate_factor_codes(dhandle, nlevels, 1000, false);
            }
        );
    }

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        handle.createDataSet("foo", H5::PredType::NATIVE_UINT8, H5S_SCALAR);
    }
    {
        H5::H5File handle(path, H5F_ACC_RDONLY);
        auto dhandle = handle.openDataSet("foo");
        expect_error(
            "1-dimensional",
            [&]() -> void {
                takane::validate_factor_codes(dhandle, nlevels, 1000, false);
            }
        );
    }

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        const std::size_t num = 99;
        std::vector<int> stuff(num);
        for (std::size_t i = 0; i < num; ++i) {
            stuff[i] = i % 10;
        }
        auto dhandle = add_hdf5_dataset(handle, "blah", H5::PredType::NATIVE_UINT16, stuff.size());
        dhandle.write(stuff.data(), H5::PredType::NATIVE_INT);
    }
    {
        H5::H5File handle(path, H5F_ACC_RDONLY);
        auto dhandle = handle.openDataSet("blah");
        expect_error(
            "less than the number",
            [&]() -> void {
                takane::validate_factor_codes(dhandle, nlevels, 1000, false);
            }
        );
    }
}

TEST(ValidateFactorCodes, MissingError) {
    auto path = define_test_path("utils_factor");
    const std::size_t nlevels = 4;

    // Check that we actually validate the missing placeholder.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto dhandle = add_hdf5_dataset(handle, "blah", H5::PredType::NATIVE_UINT16, 10);
        dhandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_UINT8, H5S_SCALAR);
    }
    {
        H5::H5File handle(path, H5F_ACC_RDONLY);
        auto dhandle = handle.openDataSet("blah");
        expect_error(
            "same datatype",
            [&]() -> void {
                takane::validate_factor_codes(dhandle, nlevels, 1000, true);
            }
        );
    }

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        const std::size_t num = 99;
        std::vector<int> stuff(num);
        for (std::size_t i = 0; i < num; ++i) {
            stuff[i] = i % 10;
        }
        auto dhandle = add_hdf5_dataset(handle, "blah", H5::PredType::NATIVE_UINT16, stuff.size());
        dhandle.write(stuff.data(), H5::PredType::NATIVE_INT);
        dhandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_UINT16, H5S_SCALAR);
    }
    {
        H5::H5File handle(path, H5F_ACC_RDONLY);
        auto dhandle = handle.openDataSet("blah");
        expect_error(
            "less than the number",
            [&]() -> void {
                takane::validate_factor_codes(dhandle, nlevels, 1000, true);
            }
        );
    }
}
