#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "takane/dense_array.hpp"

#include "utils.h"
#include "mock_dense_array.h"

#include <string>
#include <vector>
#include <filesystem>
#include <stdexcept>

TEST(DenseArray, IntegerOkay) {
    auto dir = define_test_path("dense_array");

    {
        mock_dense_array(dir, DenseArrayType::INTEGER, { 10, 20 });
    }

    test_validate(dir);
    EXPECT_EQ(test_height(dir), 10);
    std::vector<std::size_t> expected_dims { 10, 20 };
    EXPECT_EQ(test_dimensions(dir), expected_dims);
}

TEST(DenseArray, IntegerError) {
    auto dir = define_test_path("dense_array");

    {
        auto ghandle = mock_dense_array(dir, DenseArrayType::NUMBER, { 30, 10, 20 });
        ghandle.removeAttr("type");
        add_hdf5_attribute(ghandle, "type", "integer");
    }

    expect_validation_error(dir, "32-bit signed integer");
}

TEST(DenseArray, BooleanOkay) {
    auto dir = define_test_path("dense_array");

    {
        mock_dense_array(dir, DenseArrayType::BOOLEAN, { 5, 10, 20 });
    }

    test_validate(dir);
    EXPECT_EQ(test_height(dir), 5);
    std::vector<std::size_t> expected_dims { 5, 10, 20 };
    EXPECT_EQ(test_dimensions(dir), expected_dims);
}

TEST(DenseArray, BooleanError) {
    auto dir = define_test_path("dense_array");

    {
        auto ghandle = mock_dense_array(dir, DenseArrayType::NUMBER, { 8 });
        ghandle.removeAttr("type");
        add_hdf5_attribute(ghandle, "type", "boolean");
    }

    expect_validation_error(dir, "32-bit signed integer");
}

TEST(DenseArray, NumberOkay) {
    auto dir = define_test_path("dense_array");

    {
        mock_dense_array(dir, DenseArrayType::BOOLEAN, { 300 });
    }

    test_validate(dir);
    EXPECT_EQ(test_height(dir), 300);
    std::vector<std::size_t> expected_dims { 300 };
    EXPECT_EQ(test_dimensions(dir), expected_dims);
}

TEST(DenseArray, NumberError) {
    auto dir = define_test_path("dense_array");

    {
        initialize_directory_simple(dir, "dense_array", "1.0");
        H5::H5File handle(dir / "array.h5", H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup("dense_array");
        constexpr hsize_t len = 100;
        ghandle.createDataSet("data", H5::PredType::NATIVE_INT64, H5::DataSpace(1, &len));
        add_hdf5_attribute(ghandle, "type", "number");
    }

    expect_validation_error(dir, "64-bit float");
}

TEST(DenseArray, NumericMissingOkay) {
    auto dir = define_test_path("dense_array");

    {
        auto ghandle = mock_dense_array(dir, DenseArrayType::INTEGER, { 20, 10 });
        auto dhandle = ghandle.openDataSet("data");
        auto ahandle = dhandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_INT32, H5S_SCALAR);
        const int val = 100;
        ahandle.write(H5::PredType::NATIVE_INT, &val);
    }

    test_validate(dir);
    EXPECT_EQ(test_height(dir), 20);
    std::vector<std::size_t> expected_dims { 20, 10 };
    EXPECT_EQ(test_dimensions(dir), expected_dims);
}

TEST(DenseArray, NumericMissingError) {
    auto dir = define_test_path("dense_array");

    // Test that the missing placeholder is actually validated.
    {
        auto ghandle = mock_dense_array(dir, DenseArrayType::INTEGER, { 20, 10 });
        auto dhandle = ghandle.openDataSet("data");
        dhandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_DOUBLE, H5S_SCALAR);
    }

    expect_validation_error(dir, "same datatype");
}

/*****************************************/

TEST(DenseArray, StringOkay) {
    auto dir = define_test_path("dense_array");

    {
        mock_dense_array(dir, DenseArrayType::STRING, { 13, 17, 11 });
    }

    test_validate(dir);
    EXPECT_EQ(test_height(dir), 13);
    std::vector<std::size_t> expected_dims { 13, 17, 11 };
    EXPECT_EQ(test_dimensions(dir), expected_dims);
}

TEST(DenseArray, StringError) {
    auto dir = define_test_path("dense_array");

    {
        auto ghandle = mock_dense_array(dir, DenseArrayType::NUMBER, { 121 });
        ghandle.removeAttr("type");
        add_hdf5_attribute(ghandle, "type", "string");
    }
    expect_validation_error(dir, "UTF-8 encoded string");

    {
        initialize_directory_simple(dir, "dense_array", "1.0");
        H5::H5File handle(dir / "array.h5", H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup("dense_array");
        add_hdf5_dataset(ghandle, "data", H5::StrType(0, H5T_VARIABLE), 20);
        add_hdf5_attribute(ghandle, "type", "string");
    }
    expect_validation_error(dir, "NULL");
}

TEST(DenseArray, StringMissingOkay) {
    auto dir = define_test_path("dense_array");

    {
        auto ghandle = mock_dense_array(dir, DenseArrayType::STRING, { 5, 6, 2 });
        auto dhandle = ghandle.openDataSet("data");
        add_hdf5_attribute(dhandle, "missing-value-placeholder", "asdasd");
    }

    test_validate(dir);
    EXPECT_EQ(test_height(dir), 5);
    std::vector<std::size_t> expected_dims { 5, 6, 2 };
    EXPECT_EQ(test_dimensions(dir), expected_dims);
}

TEST(DenseArray, StringMissingError) {
    auto dir = define_test_path("dense_array");

    // Test that the missing placeholder is actually validated.
    {
        auto ghandle = mock_dense_array(dir, DenseArrayType::STRING, { 20, 10 });
        auto dhandle = ghandle.openDataSet("data");
        dhandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_DOUBLE, H5S_SCALAR);
    }
    expect_validation_error(dir, "UTF-8 string");
}

/*****************************************/

TEST(DenseArray, VlsOkay) {
    auto dir = define_test_path("dense_array");

    {
        mock_dense_array(dir, DenseArrayType::VLS, { 33, 22, 11 });
    }

    test_validate(dir);
    EXPECT_EQ(test_height(dir), 33);
    std::vector<std::size_t> expected_dims { 33, 22, 11 };
    EXPECT_EQ(test_dimensions(dir), expected_dims);
}

TEST(DenseArray, VlsError) {
    auto dir = define_test_path("dense_array");

    {
        initialize_directory_simple(dir, "dense_array", "1.0");
        H5::H5File handle(dir / "array.h5", H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup("dense_array");
        add_hdf5_attribute(ghandle, "type", "vls");
    }
    expect_validation_error(dir, "unsupported type");

    // Check that the heap validator is called.
    {
        auto ghandle = mock_dense_array(dir, DenseArrayType::VLS, { 33, 22, 11 });
        ghandle.unlink("heap");
        add_hdf5_dataset(ghandle, "heap", H5::PredType::NATIVE_INT32, 100);
    }
    expect_validation_error(dir, "8-bit unsigned integer");

    {
        auto ghandle = mock_dense_array(dir, DenseArrayType::VLS, { 33, 22, 11 });
        ghandle.unlink("pointers");
        ghandle.createDataSet("pointers", ritsuko::cvls::define_pointer_datatype<std::uint32_t, std::uint32_t>(), H5S_SCALAR);
    }
    expect_validation_error(dir, "at least one dimension");

    {
        auto ghandle = mock_dense_array(dir, DenseArrayType::VLS, { 50, 20 });
        std::vector<ritsuko::cvls::Pointer<std::uint64_t, std::uint64_t> > pointers(1000);
        for (auto& pp : pointers) {
            pp.offset = 10000;
            pp.length = 10000;
        }
        auto phandle = ghandle.openDataSet("pointers");
        phandle.write(pointers.data(), ritsuko::cvls::define_pointer_datatype<std::uint64_t, std::uint64_t>()); 
    }
    expect_validation_error(dir, "out of range");
}

TEST(DenseArray, VlsMissingOkay) {
    auto dir = define_test_path("dense_array");

    {
        auto ghandle = mock_dense_array(dir, DenseArrayType::VLS, { 212 });
        auto phandle = ghandle.openDataSet("pointers");
        add_hdf5_attribute(phandle, "missing-value-placeholder", "asdasd");
    }

    test_validate(dir);
    EXPECT_EQ(test_height(dir), 212);
    std::vector<std::size_t> expected_dims { 212 };
    EXPECT_EQ(test_dimensions(dir), expected_dims);
}

TEST(DenseArray, VlsMissingError) {
    auto dir = define_test_path("dense_array");

    // Test that the missing placeholder is actually validated.
    {
        auto ghandle = mock_dense_array(dir, DenseArrayType::VLS, { 99 });
        auto phandle = ghandle.openDataSet("pointers");
        phandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_DOUBLE, H5S_SCALAR);
    }
    expect_validation_error(dir, "UTF-8 string");
}

/*****************************************/

TEST(DenseArray, TransposedOkay) {
    auto dir = define_test_path("dense_array");

    {
        auto ghandle = mock_dense_array(dir, DenseArrayType::INTEGER, { 10, 5, 20 });
        auto ahandle = ghandle.createAttribute("transposed", H5::PredType::NATIVE_INT8, H5S_SCALAR);
        constexpr int val = 0;
        ahandle.write(H5::PredType::NATIVE_INT, &val);
    }
    {
        test_validate(dir);
        EXPECT_EQ(test_height(dir), 10);
        std::vector<std::size_t> expected_dims { 10, 5, 20 };
        EXPECT_EQ(test_dimensions(dir), expected_dims);
    }

    {
        auto ghandle = mock_dense_array(dir, DenseArrayType::INTEGER, { 10, 5, 20 });
        auto ahandle = ghandle.createAttribute("transposed", H5::PredType::NATIVE_INT8, H5S_SCALAR);
        constexpr int val = 1;
        ahandle.write(H5::PredType::NATIVE_INT, &val);
    }
    {
        test_validate(dir);
        EXPECT_EQ(test_height(dir), 20);
        std::vector<std::size_t> expected_dims { 20, 5, 10 };
        EXPECT_EQ(test_dimensions(dir), expected_dims);
    }
}

TEST(DenseArray, TransposedError) {
    auto dir = define_test_path("dense_array");

    {
        auto ghandle = mock_dense_array(dir, DenseArrayType::INTEGER, { 10, 5, 20 });
        hsize_t foo = 10;
        H5::DataSpace dspace(1, &foo);
        ghandle.createAttribute("transposed", H5::PredType::NATIVE_INT32, dspace);
    }
    expect_validation_error(dir, "scalar");

    {
        auto ghandle = mock_dense_array(dir, DenseArrayType::INTEGER, { 10, 20 });
        add_hdf5_attribute(ghandle, "transposed", "123123");
    }
    expect_validation_error(dir, "32-bit signed integer");
}

/*****************************************/

TEST(DenseArray, NamesOkay) {
    auto dir = define_test_path("dense_array");

    // No names.
    {
        auto ghandle = mock_dense_array(dir, DenseArrayType::INTEGER, { 5, 12, 7 });
        auto nhandle = ghandle.createGroup("names");
    }
    {
        test_validate(dir);
        EXPECT_EQ(test_height(dir), 5);
        std::vector<std::size_t> expected_dims { 5, 12, 7 };
        EXPECT_EQ(test_dimensions(dir), expected_dims);
    }

    // Full names.
    {
        auto ghandle = mock_dense_array(dir, DenseArrayType::NUMBER, { 12, 5, 7 });
        auto nhandle = ghandle.createGroup("names");
        add_hdf5_dataset(nhandle, "0", H5::StrType(0, 10), 12);
        add_hdf5_dataset(nhandle, "1", H5::StrType(0, 11), 5);
        add_hdf5_dataset(nhandle, "2", H5::StrType(0, 12), 7);
    }
    {
        test_validate(dir);
        EXPECT_EQ(test_height(dir), 12);
        std::vector<std::size_t> expected_dims { 12, 5, 7 };
        EXPECT_EQ(test_dimensions(dir), expected_dims);
    }
}

TEST(DenseArray, NamesError) {
    auto dir = define_test_path("dense_array");

    // Test that the names are actually validated.
    {
        auto ghandle = mock_dense_array(dir, DenseArrayType::INTEGER, { 10, 20 });
        auto nhandle = ghandle.createGroup("names");
        add_hdf5_dataset(nhandle, "0", H5::StrType(0, 5), 20);
    }
    expect_validation_error(dir, "same length as the extent");
}

/*****************************************/

TEST(DenseArray, PreambleError) {
    auto dir = define_test_path("dense_array");

    {
        initialize_directory_simple(dir, "dense_array", "2.0");
    }
    expect_validation_error(dir, "unsupported version");

    {
        initialize_directory_simple(dir, "dense_array", "1.0");
        H5::H5File handle(dir / "array.h5", H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup("dense_array");
        ghandle.createAttribute("type", H5::PredType::NATIVE_INT, H5S_SCALAR);
    }
    expect_validation_error(dir, "UTF-8 encoded string");
}

TEST(DenseArray, DataError) {
    auto dir = define_test_path("dense_array");

    {
        initialize_directory_simple(dir, "dense_array", "1.0");
        H5::H5File handle(dir / "array.h5", H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup("dense_array");
        ghandle.createDataSet("data", H5::PredType::NATIVE_INT32, H5S_SCALAR);
        add_hdf5_attribute(ghandle, "type", "integer");
    }
    expect_validation_error(dir, "at least one dimension");

    {
        initialize_directory_simple(dir, "dense_array", "1.0");
        H5::H5File handle(dir / "array.h5", H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup("dense_array");
        add_hdf5_dataset(ghandle, "data", H5::PredType::NATIVE_INT8, 20);
        add_hdf5_attribute(ghandle, "type", "foobar");
    }
    expect_validation_error(dir, "unknown array type");
}
