#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "H5Cpp.h"

#include "utils.h"
#include "mock_atomic_vector.h"

#include <cstddef>
#include <vector>

TEST(BumpyAtomicArray, Okay) {
    auto dir = define_test_path("bumpy_atomic_array");

    {
        initialize_directory_simple(dir, "bumpy_atomic_array", "1.0");
        H5::H5File handle(dir / "partitions.h5", H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup("bumpy_atomic_array");
        add_hdf5_numeric_dataset<int>(ghandle, "lengths", H5::PredType::NATIVE_UINT32, { 4, 1, 3, 2, 2, 0, 3, 1 });
        add_hdf5_numeric_dataset<int>(ghandle, "dimensions", H5::PredType::NATIVE_UINT32, { 2, 4 });
        mock_atomic_vector(dir / "concatenated", AtomicVectorOptions(16, AtomicVectorType::INTEGER));
    }

    test_validate(dir);
    EXPECT_EQ(test_height(dir), 2);
    EXPECT_EQ(test_dimensions(dir), (std::vector<std::size_t>{ 2, 4 }));
}

TEST(BumpyAtomicArray, Error) {
    auto dir = define_test_path("bumpy_atomic_array");

    {
        initialize_directory_simple(dir, "bumpy_atomic_array", "1.0");
        H5::H5File handle(dir / "partitions.h5", H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup("bumpy_atomic_array");
        add_hdf5_numeric_dataset<int>(ghandle, "lengths", H5::PredType::NATIVE_UINT32, { 4, 1, 3, 2, 2, 0, 3, 1 });
        add_hdf5_numeric_dataset<int>(ghandle, "dimensions", H5::PredType::NATIVE_UINT32, { 2, 4 });
        mock_atomic_vector(dir / "concatenated", AtomicVectorOptions(10, AtomicVectorType::INTEGER));
    }

    expect_validation_error(dir, "sum of lengths");
}
