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
        mock_dense_array(dir, DenseArrayOptions({ 10, 20 }, DenseArrayType::INTEGER));
    }

    test_validate(dir);
    EXPECT_EQ(test_height(dir), 10);
    EXPECT_EQ(test_dimensions(dir), (std::vector<std::size_t>{ 10, 20 }));
}

TEST(DenseArray, IntegerError) {
    auto dir = define_test_path("dense_array");

    {
        auto ghandle = mock_dense_array(dir, DenseArrayOptions({ 30, 10, 20 }, DenseArrayType::NUMBER));
        ghandle.removeAttr("type");
        add_hdf5_string_attribute(ghandle, "type", "integer");
    }

    expect_validation_error(dir, "32-bit signed integer");
}

TEST(DenseArray, BooleanOkay) {
    auto dir = define_test_path("dense_array");

    {
        mock_dense_array(dir, DenseArrayOptions({ 5, 10, 20 }, DenseArrayType::BOOLEAN));
    }

    test_validate(dir);
    EXPECT_EQ(test_height(dir), 5);
    EXPECT_EQ(test_dimensions(dir), (std::vector<std::size_t>{ 5, 10, 20 }));
}

TEST(DenseArray, BooleanError) {
    auto dir = define_test_path("dense_array");

    {
        auto ghandle = mock_dense_array(dir, DenseArrayOptions({ 8 }, DenseArrayType::NUMBER));
        ghandle.removeAttr("type");
        add_hdf5_string_attribute(ghandle, "type", "boolean");
    }

    expect_validation_error(dir, "32-bit signed integer");
}

TEST(DenseArray, NumberOkay) {
    auto dir = define_test_path("dense_array");

    {
        mock_dense_array(dir, DenseArrayOptions({ 300 }, DenseArrayType::NUMBER));
    }

    test_validate(dir);
    EXPECT_EQ(test_height(dir), 300);
    EXPECT_EQ(test_dimensions(dir), (std::vector<std::size_t>{ 300 }));
}

TEST(DenseArray, NumberError) {
    auto dir = define_test_path("dense_array");

    {
        initialize_directory_simple(dir, "dense_array", "1.0");
        H5::H5File handle(dir / "array.h5", H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup("dense_array");
        ghandle.createDataSet("data", H5::PredType::NATIVE_INT64, create_hdf5_dataspace(100));
        add_hdf5_string_attribute(ghandle, "type", "number");
    }

    expect_validation_error(dir, "64-bit float");
}

TEST(DenseArray, NumericMissingOkay) {
    auto dir = define_test_path("dense_array");

    {
        auto ghandle = mock_dense_array(dir, DenseArrayOptions({ 20, 10 }, DenseArrayType::INTEGER));
        auto dhandle = ghandle.openDataSet("data");
        dhandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_INT32, H5S_SCALAR);
    }

    test_validate(dir);
    EXPECT_EQ(test_height(dir), 20);
    EXPECT_EQ(test_dimensions(dir), (std::vector<std::size_t>{ 20, 10 }));
}

TEST(DenseArray, NumericMissingError) {
    auto dir = define_test_path("dense_array");

    // Test that the missing placeholder is actually validated.
    {
        auto ghandle = mock_dense_array(dir, DenseArrayOptions({ 20, 10 }, DenseArrayType::INTEGER));
        auto dhandle = ghandle.openDataSet("data");
        dhandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_DOUBLE, H5S_SCALAR);
    }

    expect_validation_error(dir, "same datatype");
}

/*****************************************/

TEST(DenseArray, StringOkay) {
    auto dir = define_test_path("dense_array");

    {
        mock_dense_array(dir, DenseArrayOptions({ 13, 17, 11 }, DenseArrayType::STRING));
    }

    test_validate(dir);
    EXPECT_EQ(test_height(dir), 13);
    EXPECT_EQ(test_dimensions(dir), (std::vector<std::size_t>{ 13, 17, 11 }));
}

TEST(DenseArray, StringError) {
    auto dir = define_test_path("dense_array");

    {
        auto ghandle = mock_dense_array(dir, DenseArrayOptions({ 121 }, DenseArrayType::NUMBER));
        ghandle.removeAttr("type");
        add_hdf5_string_attribute(ghandle, "type", "string");
    }
    expect_validation_error(dir, "UTF-8 encoded string");

    {
        DenseArrayOptions opt({ 20 }, DenseArrayType::STRING);
        opt.string_length = 0;
        mock_dense_array(dir, opt);
    }
    expect_validation_error(dir, "NULL");
}

TEST(DenseArray, StringMissingOkay) {
    auto dir = define_test_path("dense_array");

    {
        auto ghandle = mock_dense_array(dir, DenseArrayOptions({ 5, 6, 2 }, DenseArrayType::STRING));
        auto dhandle = ghandle.openDataSet("data");
        add_hdf5_string_attribute(dhandle, "missing-value-placeholder", "asdasd");
    }

    test_validate(dir);
    EXPECT_EQ(test_height(dir), 5);
    EXPECT_EQ(test_dimensions(dir), (std::vector<std::size_t>{ 5, 6, 2 }));
}

TEST(DenseArray, StringMissingError) {
    auto dir = define_test_path("dense_array");

    // Test that the missing placeholder is actually validated.
    {
        auto ghandle = mock_dense_array(dir, DenseArrayOptions({ 20, 10 }, DenseArrayType::STRING));
        auto dhandle = ghandle.openDataSet("data");
        dhandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_DOUBLE, H5S_SCALAR);
    }
    expect_validation_error(dir, "UTF-8 string");
}

/*****************************************/

TEST(DenseArray, VlsOkay) {
    auto dir = define_test_path("dense_array");

    {
        mock_dense_array(dir, DenseArrayOptions({ 33, 22, 11 }, DenseArrayType::VLS));
    }

    test_validate(dir);
    EXPECT_EQ(test_height(dir), 33);
    EXPECT_EQ(test_dimensions(dir), (std::vector<std::size_t>{ 33, 22, 11 }));
}

TEST(DenseArray, VlsError) {
    auto dir = define_test_path("dense_array");

    {
        DenseArrayOptions opt({ 25, 10 }, DenseArrayType::VLS);
        opt.version = "1.0";
        mock_dense_array(dir, opt);
    }
    expect_validation_error(dir, "unsupported type");

    // Check that the heap validator is called.
    {
        auto ghandle = mock_dense_array(dir, DenseArrayOptions({ 33, 22, 11 },DenseArrayType::VLS));
        ghandle.unlink("heap");
        ghandle.createDataSet("heap", H5::PredType::NATIVE_INT32, create_hdf5_dataspace(100));
    }
    expect_validation_error(dir, "8-bit unsigned integer");

    {
        auto ghandle = mock_dense_array(dir, DenseArrayOptions({ 83 }, DenseArrayType::VLS));
        ghandle.unlink("pointers");
        ghandle.createDataSet("pointers", ritsuko::cvls::define_pointer_datatype<std::uint32_t, std::uint32_t>(), H5S_SCALAR);
    }
    expect_validation_error(dir, "at least one dimension");

    {
        auto ghandle = mock_dense_array(dir, DenseArrayOptions({ 50, 20 }, DenseArrayType::VLS));
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
        auto ghandle = mock_dense_array(dir, DenseArrayOptions({ 212 }, DenseArrayType::VLS));
        auto phandle = ghandle.openDataSet("pointers");
        add_hdf5_string_attribute(phandle, "missing-value-placeholder", "asdasd");
    }

    test_validate(dir);
    EXPECT_EQ(test_height(dir), 212);
    EXPECT_EQ(test_dimensions(dir), (std::vector<std::size_t>{ 212 }));
}

TEST(DenseArray, VlsMissingError) {
    auto dir = define_test_path("dense_array");

    // Test that the missing placeholder is actually validated.
    {
        auto ghandle = mock_dense_array(dir, DenseArrayOptions({ 99 }, DenseArrayType::VLS));
        auto phandle = ghandle.openDataSet("pointers");
        phandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_DOUBLE, H5S_SCALAR);
    }
    expect_validation_error(dir, "UTF-8 string");
}

/*****************************************/

TEST(DenseArray, TransposedOkay) {
    auto dir = define_test_path("dense_array");

    {
        auto ghandle = mock_dense_array(dir, DenseArrayOptions({ 10, 5, 20 }));
        add_hdf5_numeric_attribute(ghandle, "transposed", H5::PredType::NATIVE_INT8, 0);
    }
    {
        test_validate(dir);
        EXPECT_EQ(test_height(dir), 10);
        EXPECT_EQ(test_dimensions(dir), (std::vector<std::size_t>{ 10, 5, 20 }));
    }

    {
        auto ghandle = mock_dense_array(dir, DenseArrayOptions({ 10, 5, 20 }));
        add_hdf5_numeric_attribute(ghandle, "transposed", H5::PredType::NATIVE_INT8, 1);
    }
    {
        test_validate(dir);
        EXPECT_EQ(test_height(dir), 20);
        EXPECT_EQ(test_dimensions(dir), (std::vector<std::size_t>{ 20, 5, 10 }));
    }
}

TEST(DenseArray, TransposedError) {
    auto dir = define_test_path("dense_array");

    {
        auto ghandle = mock_dense_array(dir, DenseArrayOptions({ 10, 5, 20 }));
        ghandle.createAttribute("transposed", H5::PredType::NATIVE_INT32, create_hdf5_dataspace(10));
    }
    expect_validation_error(dir, "scalar");

    {
        auto ghandle = mock_dense_array(dir, DenseArrayOptions({ 10, 20 }));
        add_hdf5_string_attribute(ghandle, "transposed", "123123");
    }
    expect_validation_error(dir, "32-bit signed integer");
}

/*****************************************/

TEST(DenseArray, NamesOkay) {
    auto dir = define_test_path("dense_array");

    // No names.
    {
        auto ghandle = mock_dense_array(dir, DenseArrayOptions({ 5, 12, 7 }));
        ghandle.createGroup("names");
    }
    {
        test_validate(dir);
        EXPECT_EQ(test_height(dir), 5);
        EXPECT_EQ(test_dimensions(dir), (std::vector<std::size_t>{ 5, 12, 7 }));
    }

    // Full names.
    {
        auto ghandle = mock_dense_array(dir, DenseArrayOptions({ 12, 5, 7 }));
        auto nhandle = ghandle.createGroup("names");
        nhandle.createDataSet("0", H5::StrType(0, 10), create_hdf5_dataspace(12));
        nhandle.createDataSet("1", H5::StrType(0, 11), create_hdf5_dataspace(5));
        nhandle.createDataSet("2", H5::StrType(0, 12), create_hdf5_dataspace(7));
    }
    {
        test_validate(dir);
        EXPECT_EQ(test_height(dir), 12);
        EXPECT_EQ(test_dimensions(dir), (std::vector<std::size_t>{ 12, 5, 7 }));
    }
}

TEST(DenseArray, NamesError) {
    auto dir = define_test_path("dense_array");

    // Test that the names are actually validated.
    {
        auto ghandle = mock_dense_array(dir, DenseArrayOptions({ 10, 20 }));
        auto nhandle = ghandle.createGroup("names");
        nhandle.createDataSet("0", H5::StrType(0, 5), create_hdf5_dataspace(20));
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
        add_hdf5_string_attribute(ghandle, "type", "integer");
    }
    expect_validation_error(dir, "at least one dimension");

    {
        initialize_directory_simple(dir, "dense_array", "1.0");
        H5::H5File handle(dir / "array.h5", H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup("dense_array");
        ghandle.createDataSet("data", H5::PredType::NATIVE_INT8, create_hdf5_dataspace(20));
        add_hdf5_string_attribute(ghandle, "type", "foobar");
    }
    expect_validation_error(dir, "unknown array type");
}
