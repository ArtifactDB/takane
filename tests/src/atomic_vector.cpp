#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "utils.h"
#include "mock_atomic_vector.h"

#include "H5Cpp.h"
#include "ritsuko/ritsuko.hpp"
#include "takane/atomic_vector.hpp"

#include <string>
#include <filesystem>
#include <fstream>

TEST(AtomicVector, PreambleError) {
    auto dir = define_test_path("atomic_vector");

    initialize_directory_simple(dir, "atomic_vector", "2.0");
    expect_validation_error(dir, "unsupported version string");

    // Check that the type is correctly extracted.
    {
        mock_atomic_vector(dir, 100, AtomicVectorType::INTEGER);
        H5::H5File handle(dir / "contents.h5", H5F_ACC_RDWR);
        auto ghandle = handle.openGroup("atomic_vector");
        ghandle.removeAttr("type");
        ghandle.createAttribute("type", H5::PredType::NATIVE_INT, H5S_SCALAR);
    }
    expect_validation_error(dir, "UTF-8 encoded string");
}

TEST(AtomicVector, ValuesError) {
    auto dir = define_test_path("atomic_vector");
    std::string name = "atomic_vector";

    {
        mock_atomic_vector(dir, 50, AtomicVectorType::INTEGER);
        H5::H5File handle(dir / "contents.h5", H5F_ACC_RDWR);
        auto ghandle = handle.openGroup("atomic_vector");
        ghandle.unlink("values");
        ghandle.createDataSet("values", H5::PredType::NATIVE_INT32, H5S_SCALAR);
    }
    expect_validation_error(dir, "1-dimensional dataset");

    {
        mock_atomic_vector(dir, 50, AtomicVectorType::INTEGER);
        H5::H5File handle(dir / "contents.h5", H5F_ACC_RDWR);
        auto ghandle = handle.openGroup("atomic_vector");
        ghandle.removeAttr("type");
        add_hdf5_attribute(ghandle, "type", "foobar");
    }
    expect_validation_error(dir, "unsupported type");
}

/*****************************************/

TEST(AtomicVector, IntegerOkay) {
    auto dir = define_test_path("atomic_vector");

    {
        mock_atomic_vector(dir, 100, AtomicVectorType::INTEGER);
    }
    test_validate(dir);
    EXPECT_EQ(test_height(dir), 100);

    // Works with a smaller type.
    {
        H5::H5File handle(dir / "contents.h5", H5F_ACC_RDWR);
        auto ghandle = handle.openGroup("atomic_vector");
        ghandle.unlink("values");
        add_hdf5_dataset(ghandle, "values", H5::PredType::NATIVE_UINT16, 99);
    }
    test_validate(dir);
    EXPECT_EQ(test_height(dir), 99);
}

TEST(AtomicVector, IntegerError) {
    auto dir = define_test_path("atomic_vector");
    mock_atomic_vector(dir, 212, AtomicVectorType::INTEGER);

    {
        H5::H5File handle(dir / "contents.h5", H5F_ACC_RDWR);
        auto ghandle = handle.openGroup("atomic_vector");
        ghandle.unlink("values");
        add_hdf5_dataset(ghandle, "values", H5::PredType::NATIVE_FLOAT, 99);
    }
    expect_validation_error(dir, "32-bit signed integer");
}

/*****************************************/

TEST(AtomicVector, BooleanOkay) {
    auto dir = define_test_path("atomic_vector");

    {
        mock_atomic_vector(dir, 33, AtomicVectorType::BOOLEAN);
    }
    test_validate(dir);
    EXPECT_EQ(test_height(dir), 33);

    // Works with a smaller type.
    {
        H5::H5File handle(dir / "contents.h5", H5F_ACC_RDWR);
        auto ghandle = handle.openGroup("atomic_vector");
        ghandle.unlink("values");
        add_hdf5_dataset(ghandle, "values", H5::PredType::NATIVE_INT8, 66);
    }
    test_validate(dir);
    EXPECT_EQ(test_height(dir), 66);
}

TEST(AtomicVector, BooleanError) {
    auto dir = define_test_path("atomic_vector");
    mock_atomic_vector(dir, 100, AtomicVectorType::BOOLEAN);

    {
        H5::H5File handle(dir / "contents.h5", H5F_ACC_RDWR);
        auto ghandle = handle.openGroup("atomic_vector");
        ghandle.unlink("values");
        add_hdf5_dataset(ghandle, "values", H5::PredType::NATIVE_INT64, 100);
    }
    expect_validation_error(dir, "32-bit signed integer");
}

/*****************************************/

TEST(AtomicVector, NumberOkay) {
    auto dir = define_test_path("atomic_vector");

    {
        mock_atomic_vector(dir, 121, AtomicVectorType::NUMBER);
    }
    test_validate(dir);
    EXPECT_EQ(test_height(dir), 121);

    // Works with a smaller type.
    {
        H5::H5File handle(dir / "contents.h5", H5F_ACC_RDWR);
        auto ghandle = handle.openGroup("atomic_vector");
        ghandle.unlink("values");
        add_hdf5_dataset(ghandle, "values", H5::PredType::NATIVE_FLOAT, 123);
    }
    test_validate(dir);
    EXPECT_EQ(test_height(dir), 123);
}

TEST(AtomicVector, NumberError) {
    auto dir = define_test_path("atomic_vector");
    mock_atomic_vector(dir, 100, AtomicVectorType::NUMBER);

    {
        H5::H5File handle(dir / "contents.h5", H5F_ACC_RDWR);
        auto ghandle = handle.openGroup("atomic_vector");
        ghandle.unlink("values");
        add_hdf5_dataset(ghandle, "values", H5::PredType::NATIVE_INT64, 82);
    }
    expect_validation_error(dir, "64-bit float");
}

/*****************************************/

TEST(AtomicVector, StringOkay) {
    auto dir = define_test_path("atomic_vector");

    {
        mock_atomic_vector(dir, 189, AtomicVectorType::STRING);
    }
    test_validate(dir);
    EXPECT_EQ(test_height(dir), 189);

    // Works with variable-length strings.
    {
        H5::H5File handle(dir / "contents.h5", H5F_ACC_RDWR);
        auto ghandle = handle.openGroup("atomic_vector");
        ghandle.unlink("values");
        auto dhandle = add_hdf5_dataset(ghandle, "values", H5::StrType(0, H5T_VARIABLE), 82);
        const char* placeholder = "takane shijou";
        std::vector<const char*> payload(82, placeholder);
        dhandle.write(payload.data(), H5::StrType(0, H5T_VARIABLE));
    }
    test_validate(dir);
    EXPECT_EQ(test_height(dir), 82);

    // Works with format == none.
    {
        H5::H5File handle(dir / "contents.h5", H5F_ACC_RDWR);
        auto ghandle = handle.openGroup("atomic_vector");
        add_hdf5_attribute(ghandle, "format", "none");
    }
    test_validate(dir);
}

TEST(AtomicVector, StringFormatOkay) {
    auto dir = define_test_path("atomic_vector");

    // Checking date only, given that the same validation function is used for date-time.
    {
        mock_atomic_vector(dir, 189, AtomicVectorType::STRING);
        H5::H5File handle(dir / "contents.h5", H5F_ACC_RDWR);
        auto ghandle = handle.openGroup("atomic_vector");
        add_hdf5_attribute(ghandle, "format", "date");

        ghandle.unlink("values");
        auto dhandle = add_hdf5_dataset(ghandle, "values", H5::StrType(0, H5T_VARIABLE), 86);
        const char* placeholder = "2023-02-24";
        std::vector<const char*> payload(86, placeholder);
        dhandle.write(payload.data(), H5::StrType(0, H5T_VARIABLE));
    }
    test_validate(dir);
    EXPECT_EQ(test_height(dir), 86);
}

TEST(AtomicVector, StringValuesError) {
    auto dir = define_test_path("atomic_vector");
    mock_atomic_vector(dir, 100, AtomicVectorType::STRING);

    {
        H5::H5File handle(dir / "contents.h5", H5F_ACC_RDWR);
        auto ghandle = handle.openGroup("atomic_vector");
        ghandle.unlink("values");
        add_hdf5_dataset(ghandle, "values", H5::PredType::NATIVE_INT, 100);
    }
    expect_validation_error(dir, "represented by a UTF-8 encoded string");

    // Check that NULL pointers are validated.
    {
        H5::H5File handle(dir / "contents.h5", H5F_ACC_RDWR);
        auto ghandle = handle.openGroup("atomic_vector");
        ghandle.unlink("values");
        add_hdf5_dataset(ghandle, "values", H5::StrType(0, H5T_VARIABLE), 100);
    }
    expect_validation_error(dir, "NULL");
}

TEST(AtomicVector, StringFormatError) {
    auto dir = define_test_path("atomic_vector");
    mock_atomic_vector(dir, 100, AtomicVectorType::STRING);

    // Test that the format attribute is parsed.
    {
        H5::H5File handle(dir / "contents.h5", H5F_ACC_RDWR);
        auto ghandle = handle.openGroup("atomic_vector");
        ghandle.createAttribute("format", H5::PredType::NATIVE_INT, H5S_SCALAR);
    }
    expect_validation_error(dir, "represented by a UTF-8 encoded string");

    // Test that the format is validated.
    {
        H5::H5File handle(dir / "contents.h5", H5F_ACC_RDWR);
        auto ghandle = handle.openGroup("atomic_vector");
        ghandle.removeAttr("format");
        add_hdf5_attribute(ghandle, "format", "date");
    }
    expect_validation_error(dir, "date-formatted string");
}

/*****************************************/

TEST(AtomicVector, NumericMissingOkay) {
    auto dir = define_test_path("atomic_vector");
    mock_atomic_vector(dir, 100, AtomicVectorType::INTEGER);

    {
        H5::H5File handle(dir / "contents.h5", H5F_ACC_RDWR);
        auto ghandle = handle.openGroup("atomic_vector");
        auto dhandle = ghandle.openDataSet("values");
        dhandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_INT32, H5S_SCALAR);
    }
    test_validate(dir);
}

TEST(AtomicVector, NumericMissingError) {
    auto dir = define_test_path("atomic_vector");
    mock_atomic_vector(dir, 100, AtomicVectorType::INTEGER);

    // Test that the missing placeholder is actualy validated.
    {
        H5::H5File handle(dir / "contents.h5", H5F_ACC_RDWR);
        auto ghandle = handle.openGroup("atomic_vector");
        auto dhandle = ghandle.openDataSet("values");
        auto attr = dhandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_FLOAT, H5S_SCALAR);
    }
    expect_validation_error(dir, "missing-value-placeholder");
}

TEST(AtomicVector, StringMissingOkay) {
    auto dir = define_test_path("atomic_vector");
    mock_atomic_vector(dir, 100, AtomicVectorType::STRING);

    {
        H5::H5File handle(dir / "contents.h5", H5F_ACC_RDWR);
        auto ghandle = handle.openGroup("atomic_vector");
        auto dhandle = ghandle.openDataSet("values");
        add_hdf5_attribute(dhandle, "missing-value-placeholder", "foobar");
    }
    test_validate(dir);

    // Works correctly with a format.
    {
        H5::H5File handle(dir / "contents.h5", H5F_ACC_RDWR);
        auto ghandle = handle.openGroup("atomic_vector");
        ghandle.unlink("values");

        H5::StrType stype(0, H5T_VARIABLE);
        constexpr hsize_t len = 30;
        auto dhandle = ghandle.createDataSet("values", stype, H5::DataSpace(1, &len));
        const char* fill = "2022-02-02T22:22:22+12:00";
        std::vector<const char*> payload(len, fill);

        add_hdf5_attribute(dhandle, "format", "date-time");
        const char* placeholder = "foobar";
        payload[5] = placeholder;
        payload[10] = placeholder;
        payload[20] = placeholder;
        dhandle.write(payload.data(), stype);
    }
    test_validate(dir);
}

TEST(AtomicVector, StringMissingError) {
    auto dir = define_test_path("atomic_vector");
    mock_atomic_vector(dir, 100, AtomicVectorType::STRING);

    // Test that the missing placeholder is actualy validated.
    {
        H5::H5File handle(dir / "contents.h5", H5F_ACC_RDWR);
        auto ghandle = handle.openGroup("atomic_vector");
        auto dhandle = ghandle.openDataSet("values");
        auto attr = dhandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_FLOAT, H5S_SCALAR);
    }
    expect_validation_error(dir, "missing-value-placeholder");
}

/*****************************************/

TEST(AtomicVector, NamesOkay) {
    auto dir = define_test_path("atomic_vector");
    mock_atomic_vector(dir, 23, AtomicVectorType::INTEGER);

    {
        H5::H5File handle(dir / "contents.h5", H5F_ACC_RDWR);
        auto ghandle = handle.openGroup("atomic_vector");
        add_hdf5_dataset(ghandle, "names", H5::StrType(0, 10), 23);
    }
    test_validate(dir);
}

TEST(AtomicVector, NamesError) {
    auto dir = define_test_path("atomic_vector");
    mock_atomic_vector(dir, 23, AtomicVectorType::INTEGER);

    // Check that the validation function is called.
    {
        H5::H5File handle(dir / "contents.h5", H5F_ACC_RDWR);
        auto ghandle = handle.openGroup("atomic_vector");
        add_hdf5_dataset(ghandle, "names", H5::StrType(0, 10), 33);
    }
    expect_validation_error(dir, "same length");
}

/*****************************************/

TEST(AtomicVector, VlsOkay) {
    auto dir = define_test_path("atomic_vector");

    {
        mock_atomic_vector(dir, 55, AtomicVectorType::VLS);
    }
    test_validate(dir);
    EXPECT_EQ(test_height(dir), 55);

    // Injecting a missing value placeholder.
    {
        H5::H5File handle(dir / "contents.h5", H5F_ACC_RDWR);
        auto ghandle = handle.openGroup("atomic_vector");
        auto dhandle = ghandle.openDataSet("pointers");
        dhandle.createAttribute("missing-value-placeholder", H5::StrType(0, 10), H5S_SCALAR);
    }
    test_validate(dir);
    EXPECT_EQ(test_height(dir), 55);
}

TEST(AtomicVector, VlsHeapError) {
    auto dir = define_test_path("atomic_vector");

    // Test that we actually validate the heap dataset.
    {
        auto ghandle = mock_atomic_vector(dir, 55, AtomicVectorType::VLS);
        ghandle.unlink("heap");
        const hsize_t len = 10;
        ghandle.createDataSet("heap", H5::PredType::NATIVE_INT8, H5::DataSpace(1, &len));
    }
    expect_validation_error(dir, "8-bit unsigned integer");
}

TEST(AtomicVector, VlsPointersShapeError) {
    auto dir = define_test_path("atomic_vector");

    {
        auto ghandle = mock_atomic_vector(dir, 55, AtomicVectorType::VLS);
        ghandle.unlink("pointers");
        ghandle.createDataSet("pointers", ritsuko::cvls::define_pointer_datatype<std::uint32_t, std::uint32_t>(), H5S_SCALAR);
    }
    expect_validation_error(dir, "1-dimensional");
}

TEST(AtomicVector, VlsPointersContentError) {
    auto dir = define_test_path("atomic_vector");

    // Test that we actually validate the pointer intervals,
    // by shortening the heap so that everything's out of range.
    {
        auto ghandle = mock_atomic_vector(dir, 55, AtomicVectorType::VLS);
        ghandle.unlink("heap");
        add_hdf5_dataset(ghandle, "heap", H5::PredType::NATIVE_UINT8, 0);
    }
    expect_validation_error(dir, "out of range");
}

TEST(AtomicVector, VlsMissingError) {
    auto dir = define_test_path("atomic_vector");

    // Test that the missing value placeholder is validated.
    {
        auto ghandle = mock_atomic_vector(dir, 55, AtomicVectorType::VLS);
        auto dhandle = ghandle.openDataSet("pointers");
        dhandle.createAttribute("missing-value-placeholder", H5::StrType(0, H5T_VARIABLE), H5S_SCALAR);
    }
    expect_validation_error(dir, "NULL");
}

TEST(AtomicVector, VlsVersionError) {
    auto dir = define_test_path("atomic_vector");

    {
        mock_atomic_vector(dir, 55, AtomicVectorType::VLS);
        std::ofstream handle(dir / "OBJECT");
        handle << "{ \"type\": \"atomic_vector\", \"atomic_vector\": { \"version\": \"1.0\" } }";
    }
    expect_validation_error(dir, "unsupported type");
}
