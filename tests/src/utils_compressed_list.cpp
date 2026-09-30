#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "takane/utils_compressed_list.hpp"

#include "utils.h"
#include "mock_atomic_vector.h"
#include "mock_data_frame.h"
#include "mock_simple_list.h"

#include <string>
#include <filesystem>
#include <fstream>

static H5::Group create_partitions(const std::filesystem::path& path, const std::string& name, const std::vector<int>& lengths) {
    H5::H5File handle(path, H5F_ACC_TRUNC);
    auto ghandle = handle.createGroup(name);
    auto dhandle = add_hdf5_dataset(ghandle, "lengths", H5::PredType::NATIVE_UINT32, lengths.size());
    dhandle.write(lengths.data(), H5::PredType::NATIVE_INT);
    return ghandle;
}

TEST(ValidateCompressedList, Okay) {
    auto dir = define_test_path("utils_compressed_list");

    {
        initialize_directory_simple(dir, "atomic_vector_list", "1.0");
        create_partitions(dir / "partitions.h5", "atomic_vector_list", { 4, 2, 3, 1 });
        mock_atomic_vector(dir / "concatenated", 10, AtomicVectorType::INTEGER);
    }
    {
        auto meta = takane::read_object_metadata(dir);
        takane::validate_compressed_list<false>(dir, "atomic_vector_list", "atomic_vector", meta, {});
        EXPECT_EQ(takane::height_of_compressed_list(dir, "atomic_vector_list", meta, {}), 4);
    }

    // Trying with a different set of partitions. 
    {
        initialize_directory_simple(dir, "atomic_vector_list", "1.0");
        create_partitions(dir / "partitions.h5", "atomic_vector_list", { 1, 0, 0, 2, 0, 1, 1, 0, 3, 2, 0, 1 });
        mock_atomic_vector(dir / "concatenated", 11, AtomicVectorType::INTEGER);
    }
    {
        auto meta = takane::read_object_metadata(dir);
        takane::validate_compressed_list<false>(dir, "atomic_vector_list", "atomic_vector", meta, {});
        EXPECT_EQ(takane::height_of_compressed_list(dir, "atomic_vector_list", meta, {}), 12);
    }

    // Trying with interface satisfaction.
    {
        initialize_directory_simple(dir, "data_frame_list", "1.0");
        create_partitions(dir / "partitions.h5", "data_frame_list", { 4, 12, 9, 1, 5 });
        mock_data_frame(dir / "concatenated", 31, {});
    }
    {
        auto meta = takane::read_object_metadata(dir);
        takane::validate_compressed_list<true>(dir, "data_frame_list", "DATA_FRAME", meta, {});
        EXPECT_EQ(takane::height_of_compressed_list(dir, "data_frame_list", meta, {}), 5);
    }
}

TEST(ValidateCompressedList, GeneralError) {
    auto dir = define_test_path("utils_compressed_list");

    {
        initialize_directory_simple(dir, "atomic_vector_list", "2.0");
    }
    auto meta = takane::read_object_metadata(dir);
    expect_error(
        "unsupported version string",
        [&]() -> void {
            takane::validate_compressed_list<false>(dir, "atomic_vector_list", "atomic_vector", meta, {});
        }
    );
}

TEST(ValidateCompressedList, ConcatenatedError) {
    auto dir = define_test_path("utils_compressed_list");

    std::vector<int> lengths{ 4, 2, 3, 1 };

    // Not a derived object.
    {
        initialize_directory_simple(dir, "atomic_vector_list", "1.0");
        create_partitions(dir / "partitions.h5", "atomic_vector_list", lengths);
        mock_data_frame(dir / "concatenated", 10, {});
    }
    {
        auto meta = takane::read_object_metadata(dir);
        expect_error(
            "contain an object of type 'atomic_vector'",
            [&]() -> void {
                takane::validate_compressed_list<false>(dir, "atomic_vector_list", "atomic_vector", meta, {});
            }
        );
    }

    // Fails to satsify the interface.
    {
        initialize_directory_simple(dir, "data_frame_list", "1.0");
        create_partitions(dir / "partitions.h5", "atomic_vector_list", lengths);
        mock_atomic_vector(dir / "concatenated", 10, AtomicVectorType::INTEGER);
    }
    {
        auto meta = takane::read_object_metadata(dir);
        expect_error(
            "satisfy the 'DATA_FRAME' interface",
            [&]() -> void {
                takane::validate_compressed_list<true>(dir, "data_frame_list", "DATA_FRAME", meta, {});
            }
        );
    }

    // Validation fails.
    {
        initialize_directory_simple(dir, "atomic_vector_list", "1.0");
        create_partitions(dir / "partitions.h5", "atomic_vector_list", lengths);
        auto ghandle2 = mock_atomic_vector(dir / "concatenated", 10, AtomicVectorType::INTEGER);
        ghandle2.removeAttr("type");
        add_hdf5_attribute(ghandle2, "type", "string");
    }
    {
        auto meta = takane::read_object_metadata(dir);
        expect_error(
            "UTF-8 encoded string",
            [&]() -> void {
                takane::validate_compressed_list<false>(dir, "atomic_vector_list", "atomic_vector", meta, {});
            }
        );
    }
}

TEST(ValidateCompressedList, PartitionsError) {
    auto dir = define_test_path("utils_compressed_list");

    {
        initialize_directory_simple(dir, "atomic_vector_list", "1.0");
        H5::H5File handle(dir / "partitions.h5", H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup("atomic_vector_list");
        add_hdf5_dataset(ghandle, "lengths", H5::PredType::NATIVE_INT32, 20);
        mock_atomic_vector(dir / "concatenated", 10, AtomicVectorType::INTEGER);
    }
    {
        auto meta = takane::read_object_metadata(dir);
        expect_error(
            "64-bit unsigned integer",
            [&]() -> void {
                takane::validate_compressed_list<false>(dir, "atomic_vector_list", "atomic_vector", meta, {});
            }
        );
    }

    {
        initialize_directory_simple(dir, "atomic_vector_list", "1.0");
        H5::H5File handle(dir / "partitions.h5", H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup("atomic_vector_list");
        ghandle.createDataSet("lengths", H5::PredType::NATIVE_UINT32, H5S_SCALAR);
        mock_atomic_vector(dir / "concatenated", 10, AtomicVectorType::INTEGER);
    }
    {
        auto meta = takane::read_object_metadata(dir);
        expect_error(
            "1-dimensional",
            [&]() -> void {
                takane::validate_compressed_list<false>(dir, "atomic_vector_list", "atomic_vector", meta, {});
            }
        );
    }

    {
        initialize_directory_simple(dir, "atomic_vector_list", "1.0");
        create_partitions(dir / "partitions.h5", "atomic_vector_list", { 4, 2, 3, 1 });
        mock_atomic_vector(dir / "concatenated", 20, AtomicVectorType::INTEGER);
    }
    {
        auto meta = takane::read_object_metadata(dir);
        expect_error(
            "sum of 'lengths'",
            [&]() -> void {
                takane::validate_compressed_list<false>(dir, "atomic_vector_list", "atomic_vector", meta, {});
            }
        );
    }
}

TEST(ValidateCompressedList, Names) {
    auto dir = define_test_path("utils_compressed_list");

    {
        initialize_directory_simple(dir, "atomic_vector_list", "1.0");
        auto ghandle = create_partitions(dir / "partitions.h5", "atomic_vector_list", { 1, 2, 3, 4, 5, 6 });
        add_hdf5_dataset(ghandle, "names", H5::StrType(0, 5), 6);
        mock_atomic_vector(dir / "concatenated", 21, AtomicVectorType::INTEGER);
    }
    {
        auto meta = takane::read_object_metadata(dir);
        takane::validate_compressed_list<false>(dir, "atomic_vector_list", "atomic_vector", meta, {});
        EXPECT_EQ(takane::height_of_compressed_list(dir, "atomic_vector_list", meta, {}), 6);
    }

    // Test that some kind of validation is performed.
    {
        H5::H5File handle(dir / "partitions.h5", H5F_ACC_RDWR);
        auto ghandle = handle.openGroup("atomic_vector_list");
        ghandle.unlink("names");
        add_hdf5_dataset(ghandle, "names", H5::StrType(0, 5), 10);
    }
    {
        auto meta = takane::read_object_metadata(dir);
        expect_error(
            "same length as",
            [&]() -> void {
                takane::validate_compressed_list<false>(dir, "atomic_vector_list", "atomic_vector", meta, {});
            }
        );
    }
}

TEST(ValidateCompressedList, Metadata) {
    auto dir = define_test_path("utils_compressed_list");

    {
        initialize_directory_simple(dir, "atomic_vector_list", "1.0");
        create_partitions(dir / "partitions.h5", "atomic_vector_list", { 3, 3, 2, 2, 1, 1, 0, 0 });
        mock_atomic_vector(dir / "concatenated", 12, AtomicVectorType::INTEGER);
        mock_simple_list(dir / "other_annotations");
    }
    {
        auto meta = takane::read_object_metadata(dir);
        takane::validate_compressed_list<false>(dir, "atomic_vector_list", "atomic_vector", meta, {});
        EXPECT_EQ(takane::height_of_compressed_list(dir, "atomic_vector_list", meta, {}), 8);
    }

    // Test that some kind of validation is performed.
    {
        std::filesystem::remove_all(dir / "other_annotations");
        mock_atomic_vector(dir / "other_annotations", 12, AtomicVectorType::INTEGER);
    }
    {
        auto meta = takane::read_object_metadata(dir);
        expect_error(
            "'SIMPLE_LIST' interface",
            [&]() -> void {
                takane::validate_compressed_list<false>(dir, "atomic_vector_list", "atomic_vector", meta, {});
            }
        );
    }
}

TEST(ValidateCompressedList, Mcols) {
    auto dir = define_test_path("utils_compressed_list");

    {
        initialize_directory_simple(dir, "atomic_vector_list", "1.0");
        create_partitions(dir / "partitions.h5", "atomic_vector_list", { 3, 3, 2, 2, 1, 1, 0, 0 });
        mock_atomic_vector(dir / "concatenated", 12, AtomicVectorType::INTEGER);
        mock_data_frame(dir / "element_annotations", 8, {});
    }
    {
        auto meta = takane::read_object_metadata(dir);
        takane::validate_compressed_list<false>(dir, "atomic_vector_list", "atomic_vector", meta, {});
        EXPECT_EQ(takane::height_of_compressed_list(dir, "atomic_vector_list", meta, {}), 8);
    }

    // Test that some kind of validation is performed.
    {
        std::filesystem::remove_all(dir / "element_annotations");
        mock_data_frame(dir / "element_annotations", 9, {});
    }
    {
        auto meta = takane::read_object_metadata(dir);
        expect_error(
            "number of rows",
            [&]() -> void {
                takane::validate_compressed_list<false>(dir, "atomic_vector_list", "atomic_vector", meta, {});
            }
        );
    }
}
