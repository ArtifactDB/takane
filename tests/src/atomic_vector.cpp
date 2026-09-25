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

// PURGE ME //
static void test_validate(const std::filesystem::path& dir) {
    takane::validate_atomic_vector(dir, takane::read_object_metadata(dir), {});
}

static std::size_t test_height(const std::filesystem::path& dir) {
    return takane::height_of_atomic_vector(dir, takane::read_object_metadata(dir), {});
}
// PURGE ME //

static void expect_error(const std::filesystem::path& dir, const std::string& msg) {
    std::string err;
    try {
        test_validate(dir);
    } catch (std::exception& e) {
        err = e.what();
    }
    EXPECT_THAT(err, ::testing::HasSubstr(msg));
}

/*****************************************/

TEST(AtomicVector, PreambleError) {
    auto dir = define_test_path("atomic_vector");

    initialize_directory_simple(dir, "atomic_vector", "2.0");
    expect_error(dir, "unsupported version string");

    // Check that the type is correctly extracted.
    {
        mock_atomic_vector(dir, 100, AtomicVectorType::INTEGER);
        H5::H5File handle(dir / "contents.h5", H5F_ACC_RDWR);
        auto ghandle = handle.openGroup("atomic_vector");
        ghandle.removeAttr("type");
        ghandle.createAttribute("type", H5::PredType::NATIVE_INT, H5S_SCALAR);
    }
    expect_error(dir, "UTF-8 encoded string");
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
    expect_error(dir, "1-dimensional dataset");

    {
        mock_atomic_vector(dir, 50, AtomicVectorType::INTEGER);
        H5::H5File handle(dir / "contents.h5", H5F_ACC_RDWR);
        auto ghandle = handle.openGroup("atomic_vector");
        ghandle.removeAttr("type");
        add_hdf5_attribute(ghandle, "type", "foobar");
    }
    expect_error(dir, "unsupported type");
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
    expect_error(dir, "32-bit signed integer");
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
    expect_error(dir, "32-bit signed integer");
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
    expect_error(dir, "64-bit float");
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
    expect_error(dir, "represented by a UTF-8 encoded string");

    // Check that NULL pointers are validated.
    {
        H5::H5File handle(dir / "contents.h5", H5F_ACC_RDWR);
        auto ghandle = handle.openGroup("atomic_vector");
        ghandle.unlink("values");
        add_hdf5_dataset(ghandle, "values", H5::StrType(0, H5T_VARIABLE), 100);
    }
    expect_error(dir, "NULL");
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
    expect_error(dir, "represented by a UTF-8 encoded string");

    // Test that the format is validated.
    {
        H5::H5File handle(dir / "contents.h5", H5F_ACC_RDWR);
        auto ghandle = handle.openGroup("atomic_vector");
        ghandle.removeAttr("format");
        add_hdf5_attribute(ghandle, "format", "date");
    }
    expect_error(dir, "date-formatted string");
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
    expect_error(dir, "missing-value-placeholder");
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
    expect_error(dir, "missing-value-placeholder");
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
    expect_error(dir, "same length");
}

/*****************************************/

static void mock_vls_atomic_vector(const std::filesystem::path& dir) {
    initialize_directory_simple(dir, "atomic_vector", "1.1");
    H5::H5File handle(dir / "contents.h5", H5F_ACC_TRUNC);
    auto ghandle = handle.createGroup("atomic_vector");
    add_hdf5_attribute(ghandle, "type", "vls");

    const std::string heap = "abcdefghijklmno";
    std::vector<std::uint8_t> buffer(heap.size());
    std::copy(heap.begin(), heap.end(), reinterpret_cast<char*>(buffer.data()));
    hsize_t hlen = heap.size();
    auto hhandle = ghandle.createDataSet("heap", H5::PredType::NATIVE_UINT8, H5::DataSpace(1, &hlen));
    hhandle.write(buffer.data(), H5::PredType::NATIVE_UINT8);

    std::vector<ritsuko::cvls::Pointer<uint64_t, std::uint64_t> > pointers(3);
    pointers[0].offset = 0; pointers[0].length = 5;
    pointers[1].length = 5; pointers[1].length = 7;
    pointers[1].length = 12; pointers[1].length = 3;

    hsize_t plen = pointers.size();
    H5::DataSpace pspace(1, &plen);
    auto ptype = ritsuko::cvls::define_pointer_datatype<std::uint64_t, std::uint64_t>();
    auto phandle = ghandle.createDataSet("pointers", ptype, pspace);
    phandle.write(pointers.data(), ptype);
}

TEST(AtomicVector, Vls) {
    auto dir = define_test_path("atomic_vector");

    {
        mock_vls_atomic_vector(dir);
    }
    test_validate(dir);
    EXPECT_EQ(test_height(dir), 3);

    // Injecting a missing value placeholder.
    {
        H5::H5File handle(dir / "contents.h5", H5F_ACC_RDWR);
        auto ghandle = handle.openGroup("atomic_vector");
        auto dhandle = ghandle.openDataSet("pointers");
        dhandle.createAttribute("missing-value-placeholder", H5::StrType(0, 10), H5S_SCALAR);
    }
    test_validate(dir);
}

TEST(AtomicVector, VlsHeapError) {
    auto dir = define_test_path("atomic_vector");

    // Test that we actually validate the heap dataset.
    {
        mock_vls_atomic_vector(dir);
        H5::H5File handle(dir / "contents.h5", H5F_ACC_RDWR);
        auto ghandle = handle.openGroup("atomic_vector");
        ghandle.unlink("heap");
        const hsize_t len = 10;
        ghandle.createDataSet("heap", H5::PredType::NATIVE_INT8, H5::DataSpace(1, &len));
    }
    expect_error(dir, "8-bit unsigned integer");
}

TEST(AtomicVector, VlsPointersShapeError) {
    auto dir = define_test_path("atomic_vector");

    {
        mock_vls_atomic_vector(dir);
        H5::H5File handle(dir / "contents.h5", H5F_ACC_RDWR);
        auto ghandle = handle.openGroup("atomic_vector");
        ghandle.unlink("pointers");
        ghandle.createDataSet("pointers", ritsuko::cvls::define_pointer_datatype<std::uint32_t, std::uint32_t>(), H5S_SCALAR);
    }
    expect_error(dir, "1-dimensional");
}

TEST(AtomicVector, VlsPointersContentError) {
    auto dir = define_test_path("atomic_vector");

    // Test that we actually validate the pointer intervals,
    // by shortening the heap so that everything's out of range.
    {
        mock_vls_atomic_vector(dir);
        H5::H5File handle(dir / "contents.h5", H5F_ACC_RDWR);
        auto ghandle = handle.openGroup("atomic_vector");
        ghandle.unlink("heap");
        hsize_t zero = 0;
        H5::DataSpace hspace(1, &zero);
        ghandle.createDataSet("heap", H5::PredType::NATIVE_UINT8, hspace);
    }
    expect_error(dir, "out of range");
}

TEST(AtomicVector, VlsMissingError) {
    auto dir = define_test_path("atomic_vector");

    // Test that the missing value placeholder is validated.
    {
        mock_vls_atomic_vector(dir);
        H5::H5File handle(dir / "contents.h5", H5F_ACC_RDWR);
        auto ghandle = handle.openGroup("atomic_vector");
        auto dhandle = ghandle.openDataSet("pointers");
        dhandle.createAttribute("missing-value-placeholder", H5::StrType(0, H5T_VARIABLE), H5S_SCALAR);
    }
    expect_error(dir, "NULL");
}

TEST(AtomicVector, VlsVersionError) {
    auto dir = define_test_path("atomic_vector");

    {
        mock_vls_atomic_vector(dir);
        auto opath = dir/"OBJECT";
        auto parsed = millijson::parse_file(opath.c_str(), {});
        auto& entries = reinterpret_cast<millijson::Object*>(parsed.get())->value();
        auto& av_entries = reinterpret_cast<millijson::Object*>(entries["atomic_vector"].get())->value();
        reinterpret_cast<millijson::String*>(av_entries["version"].get())->value() = "1.0";
        dump_json(parsed.get(), opath);
    }
    expect_error(dir, "unsupported type");
}
