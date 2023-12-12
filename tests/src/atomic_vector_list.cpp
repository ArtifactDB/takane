#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "takane/takane.hpp"
#include "utils.h"
#include "atomic_vector.h"
#include "data_frame.h"
#include "simple_list.h"

#include <string>
#include <filesystem>
#include <fstream>

struct AtomicVectorListTest : public::testing::Test {
    AtomicVectorListTest() {
        dir = "TEST_atomic_vector_list";
        name = "atomic_vector_list";
    }

    std::filesystem::path dir;
    std::string name;

    H5::H5File initialize() {
        initialize_directory_simple(dir, name, "1.0");
        return H5::H5File(dir / "partitions.h5", H5F_ACC_TRUNC);
    }

    H5::H5File reopen() {
        return H5::H5File(dir / "partitions.h5", H5F_ACC_RDWR);
    }

    void expect_error(const std::string& msg) {
        EXPECT_ANY_THROW({
            try {
                takane::validate(dir);
            } catch (std::exception& e) {
                EXPECT_THAT(e.what(), ::testing::HasSubstr(msg));
                throw;
            }
        });
    }
};

TEST_F(AtomicVectorListTest, Basic) {
    initialize_directory_simple(dir, name, "2.0");
    expect_error("unsupported version string");

    {
        auto handle = initialize();
        auto ghandle = handle.createGroup(name);
        hdf5_utils::spawn_numeric_data<int>(ghandle, "lengths", H5::PredType::NATIVE_UINT32, { 4, 3, 2, 1 });
        initialize_directory_simple(dir / "concatenated", "foobar", "1.0");
    }
    expect_error("should contain an 'atomic_vector'");

    {
        initialize_directory_simple(dir / "concatenated", "atomic_vector", "1.0");
    }
    expect_error("failed to validate the 'concatenated'");

    {
        atomic_vector::mock(dir / "concatenated", 7, atomic_vector::Type::INTEGER);
    }
    expect_error("sum of 'lengths'");

    {
        atomic_vector::mock(dir / "concatenated", 10, atomic_vector::Type::INTEGER);
    }
    takane::validate(dir);
    EXPECT_EQ(takane::height(dir), 4);
}
