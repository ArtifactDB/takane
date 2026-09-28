#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "utils.h"
#include "atomic_vector.h"
#include "data_frame.h"
#include "simple_list.h"

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

static void expect_validation_error(const std::filesystem::path& dir, const std::string& msg) {
    std::string err;
    try {
        test_validate(dir);
    } catch (std::exception& e) {
        err = e.what();
    }
    EXPECT_THAT(err, ::testing::HasSubstr(msg));
}
// PURGE ME //

TEST(AtomicVectorList, Okay) {
    auto dir = define_test_path("atomic_vector_list");

    {
        H5::H5File handle(dir / "partitions.h5", H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup("atomic_vector_length");
        hdf5_utils::spawn_numeric_data<int>(ghandle, "lengths", H5::PredType::NATIVE_UINT32, { 4, 3, 2, 1 });
        mock_atomic_vector(dir / "concatenated", 10, atomic_vector::Type::INTEGER);
    }
    test_validate(dir);
    EXPECT_EQ(test_height(dir), 4);

    // Works with other vector types.
    mock_atomic_vector(dir / "concatenated", 10, atomic_vector::Type::STRING);
    test_validate(dir);
    EXPECT_EQ(test_height(dir), 4);
}

TEST(AtomicVectorList, Error) {
    initialize_directory_simple(dir, name, "2.0");
    expect_error("unsupported version string");

    {
        H5::H5File handle(dir / "partitions.h5", H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup("atomic_vector_list");
        hdf5_utils::spawn_numeric_data<int>(ghandle, "lengths", H5::PredType::NATIVE_UINT32, { 4, 3, 2, 1 });
        initialize_directory_simple(dir / "concatenated", "foobar", "1.0");
    }
    expect_error("should contain an 'atomic_vector'");

    {
        initialize_directory_simple(dir / "concatenated", "atomic_vector", "2.0");
    }
    expect_error("failed to validate the 'concatenated'");

    {
        atomic_vector::mock(dir / "concatenated", 7, atomic_vector::Type::INTEGER);
    }
    expect_error("sum of 'lengths'");
}
