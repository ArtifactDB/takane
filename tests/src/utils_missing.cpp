#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "takane/utils_missing.hpp"
#include "utils.h"

TEST(CheckStringMissingPlaceholder, Okay) {
    auto dir = define_test_path("utils_json");
    auto path = dir / "thing.h5";

    // No-op if it's missing.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        add_hdf5_dataset(handle, "foo", H5::StrType(0, 10), 20);
    }
    {
        H5::H5File handle(path, H5F_ACC_RDONLY);
        auto dhandle = handle.openDataSet("foo");
        takane::validate_string_missing_placeholder(dhandle, "missing-value-placeholder");
        auto val = takane::read_string_missing_placeholder(dhandle, "missing-value-placeholder");
        EXPECT_FALSE(val.has_value());
    }

    // Actually get the placeholder.
    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        auto dhandle = handle.openDataSet("foo");
        add_hdf5_attribute(dhandle, "missing-value-placeholder", "foobar");
    }
    {
        H5::H5File handle(path, H5F_ACC_RDONLY);
        auto dhandle = handle.openDataSet("foo");
        takane::validate_string_missing_placeholder(dhandle, "missing-value-placeholder");
        auto val = takane::read_string_missing_placeholder(dhandle, "missing-value-placeholder");
        ASSERT_TRUE(val.has_value());
        EXPECT_EQ(*val, "foobar");
    }
}

TEST(CheckStringMissingPlaceholder, Error) {
    auto dir = define_test_path("utils_json");
    auto path = dir / "thing.h5";

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto dhandle = add_hdf5_dataset(handle, "foo", H5::StrType(0, 10), 20);
        constexpr hsize_t one = 1;
        dhandle.createAttribute("missing-value-placeholder", H5::StrType(0, 4), H5::DataSpace(1, &one));
    }
    {
        H5::H5File handle(path, H5F_ACC_RDONLY);
        auto dhandle = handle.openDataSet("foo");
        expect_error(
            "scalar",
            [&]() -> void {
                takane::validate_string_missing_placeholder(dhandle, "missing-value-placeholder");
            }
        );
        expect_error(
            "scalar",
            [&]() -> void {
                takane::read_string_missing_placeholder(dhandle, "missing-value-placeholder");
            }
        );
    }

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto dhandle = add_hdf5_dataset(handle, "foo", H5::StrType(0, 10), 20);
        dhandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_INT, H5S_SCALAR);
    }
    {
        H5::H5File handle(path, H5F_ACC_RDONLY);
        auto dhandle = handle.openDataSet("foo");
        expect_error(
            "UTF-8 string",
            [&]() -> void {
                takane::validate_string_missing_placeholder(dhandle, "missing-value-placeholder");
            }
        );
        expect_error(
            "UTF-8 string",
            [&]() -> void {
                takane::read_string_missing_placeholder(dhandle, "missing-value-placeholder");
            }
        );
    }
}

TEST(CheckNumericMissingPlaceholder, Okay) {
    auto dir = define_test_path("utils_json");
    auto path = dir / "thing.h5";

    // No-op if it's missing.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        add_hdf5_dataset(handle, "foo", H5::PredType::NATIVE_INT32, 20);
    }
    {
        H5::H5File handle(path, H5F_ACC_RDONLY);
        auto dhandle = handle.openDataSet("foo");
        takane::validate_numeric_missing_placeholder(dhandle, "missing-value-placeholder");
        EXPECT_FALSE(takane::read_numeric_missing_placeholder<int>(dhandle, "missing-value-placeholder").has_value());
    }

    // Actually check the placeholder.
    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        auto dhandle = handle.openDataSet("foo");
        auto ahandle = dhandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_INT32, H5S_SCALAR); 
        const int val = 99;
        ahandle.write(H5::PredType::NATIVE_INT, &val);
    }
    {
        H5::H5File handle(path, H5F_ACC_RDONLY);
        auto dhandle = handle.openDataSet("foo");
        takane::validate_numeric_missing_placeholder(dhandle, "missing-value-placeholder");

        auto val = takane::read_numeric_missing_placeholder<int>(dhandle, "missing-value-placeholder");
        ASSERT_TRUE(val.has_value());
        EXPECT_EQ(*val, 99);
    }

    // Try a floating-point dataset. 
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto dhandle = add_hdf5_dataset(handle, "foo", H5::PredType::NATIVE_DOUBLE, 20);
        auto ahandle = dhandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_DOUBLE, H5S_SCALAR); 
        const double val = 0.5;
        ahandle.write(H5::PredType::NATIVE_DOUBLE, &val);
    }
    {
        H5::H5File handle(path, H5F_ACC_RDONLY);
        auto dhandle = handle.openDataSet("foo");
        takane::validate_numeric_missing_placeholder(dhandle, "missing-value-placeholder");

        auto val = takane::read_numeric_missing_placeholder<double>(dhandle, "missing-value-placeholder");
        ASSERT_TRUE(val.has_value());
        EXPECT_EQ(*val, 0.5);
    }
}

TEST(CheckNumericMissingPlaceholder, Error) {
    auto dir = define_test_path("utils_json");
    auto path = dir / "thing.h5";

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto dhandle = add_hdf5_dataset(handle, "foo", H5::PredType::NATIVE_INT32, 20);
        constexpr hsize_t one = 1;
        dhandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_INT32, H5::DataSpace(1, &one));
    }
    {
        H5::H5File handle(path, H5F_ACC_RDONLY);
        auto dhandle = handle.openDataSet("foo");
        expect_error(
            "scalar",
            [&]() -> void {
                takane::validate_numeric_missing_placeholder(dhandle, "missing-value-placeholder");
            }
        );
        expect_error(
            "scalar",
            [&]() -> void {
                takane::read_numeric_missing_placeholder<int>(dhandle, "missing-value-placeholder");
            }
        );
    }

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto dhandle = add_hdf5_dataset(handle, "foo", H5::PredType::NATIVE_INT32, 20);
        dhandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_INT8, H5S_SCALAR);
    }
    {
        H5::H5File handle(path, H5F_ACC_RDONLY);
        auto dhandle = handle.openDataSet("foo");
        expect_error(
            "same datatype",
            [&]() -> void {
                takane::validate_numeric_missing_placeholder(dhandle, "missing-value-placeholder");
            }
        );
        expect_error(
            "same datatype",
            [&]() -> void {
                takane::read_numeric_missing_placeholder<int>(dhandle, "missing-value-placeholder");
            }
        );
    }
}
