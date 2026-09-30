#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "utils.h"
#include "mock_atomic_vector.h"
#include "mock_compressed_list.h"

#include "H5Cpp.h"
#include "takane/atomic_vector_list.hpp"

#include <string>
#include <filesystem>
#include <fstream>

// These tests are cursory and just check that the correct arguments are passed to validate_compressed_list().
// Most of the heavy testing is actually performed in utils_compressed_list.cpp.

TEST(AtomicVectorList, Okay) {
    auto dir = define_test_path("atomic_vector_list");

    {
        initialize_directory_simple(dir, "atomic_vector_list", "1.0"); 
        mock_compressed_list_partitions(dir / "partitions.h5", "atomic_vector_list", { 0, 4, 3, 2, 1 });
        mock_atomic_vector(dir / "concatenated", 10, AtomicVectorType::INTEGER);
    }
    test_validate(dir);
    EXPECT_EQ(test_height(dir), 5);
}

TEST(AtomicVectorList, Error) {
    auto dir = define_test_path("atomic_vector_list");

    {
        initialize_directory_simple(dir, "atomic_vector_list", "1.0"); 
        mock_compressed_list_partitions(dir / "partitions.h5", "atomic_vector_list", { 4, 3, 2, 1 });
        initialize_directory_simple(dir / "concatenated", "foobar", "1.0");
    }
    expect_validation_error(dir, "should contain an object of type 'atomic_vector'");
}
