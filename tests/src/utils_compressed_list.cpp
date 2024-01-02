#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "takane/utils_compressed_list.hpp"
#include "utils.h"
#include "atomic_vector.h"
#include "data_frame.h"
#include "simple_list.h"

#include <string>
#include <filesystem>
#include <fstream>

struct CompressedListUtilsTest : public::testing::Test {
    CompressedListUtilsTest() {
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

    template<bool satisfactory = false>
    void expect_error(const std::string& msg) {
        auto meta = takane::read_object_metadata(dir);
        EXPECT_ANY_THROW({
            try {
                takane::internal_compressed_list::validate_directory<satisfactory>(dir, name, "atomic_vector", meta, takane::Options());
            } catch (std::exception& e) {
                EXPECT_THAT(e.what(), ::testing::HasSubstr(msg));
                throw;
            }
        });
    }
};

TEST_F(CompressedListUtilsTest, Basic) {
    initialize_directory_simple(dir, name, "2.0");
    expect_error("unsupported version string");

    {
        auto handle = initialize();
        auto ghandle = handle.createGroup(name);
        hdf5_utils::spawn_numeric_data<int>(ghandle, "lengths", H5::PredType::NATIVE_UINT32, { 4, 3, 2, 1 });
        initialize_directory_simple(dir / "concatenated", "foobar", "1.0");
    }
    expect_error("should contain an 'atomic_vector'");
    expect_error<true>("'atomic_vector' interface");

    initialize_directory_simple(dir / "concatenated", "atomic_vector", "1.0");
    expect_error("failed to validate the 'concatenated'");

    // Success at last!
    {
        atomic_vector::mock(dir / "concatenated", 10, atomic_vector::Type::INTEGER);
    }
    auto meta = takane::read_object_metadata(dir);
    takane::internal_compressed_list::validate_directory<false>(dir, "atomic_vector_list", "atomic_vector", meta, takane::Options());
    EXPECT_EQ(takane::internal_compressed_list::height(dir, name, meta, takane::Options()), 4);
}

TEST_F(CompressedListUtilsTest, Lengths) {
    {
        auto handle = initialize();
        auto ghandle = handle.createGroup(name);
        hdf5_utils::spawn_numeric_data<int>(ghandle, "lengths", H5::PredType::NATIVE_INT32, { 4, 3, 2, 1 });
        atomic_vector::mock(dir / "concatenated", 10, atomic_vector::Type::INTEGER);
    }
    expect_error("64-bit unsigned integer");

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        ghandle.unlink("lengths");
        hdf5_utils::spawn_numeric_data<int>(ghandle, "lengths", H5::PredType::NATIVE_UINT8, { 4, 3, 2, 1 });
        atomic_vector::mock(dir / "concatenated", 7, atomic_vector::Type::INTEGER);
    }
    expect_error("sum of 'lengths'");
}

TEST_F(CompressedListUtilsTest, Names) {
    {
        auto handle = initialize();
        auto ghandle = handle.createGroup(name);
        hdf5_utils::spawn_numeric_data<int>(ghandle, "lengths", H5::PredType::NATIVE_UINT32, { 4, 3, 2, 1 });
        atomic_vector::mock(dir / "concatenated", 10, atomic_vector::Type::NUMBER);

        hdf5_utils::spawn_string_data(ghandle, "names", H5T_VARIABLE, { "Aaron", "Charlie", "Echo", "Fooblewooble" });
    }
    auto meta = takane::read_object_metadata(dir);
    takane::internal_compressed_list::validate_directory<false>(dir, "atomic_vector_list", "atomic_vector", meta, takane::Options());

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        ghandle.unlink("names");
        hdf5_utils::spawn_string_data(ghandle, "names", H5T_VARIABLE, { "Aaron" });
    }
    expect_error("same length");
}

TEST_F(CompressedListUtilsTest, Metadata) {
    {
        auto handle = initialize();
        auto ghandle = handle.createGroup(name);
        hdf5_utils::spawn_numeric_data<int>(ghandle, "lengths", H5::PredType::NATIVE_UINT32, { 4, 3, 2, 1 });
        atomic_vector::mock(dir / "concatenated", 10, atomic_vector::Type::BOOLEAN);
    }

    auto cdir = dir / "element_annotations";
    auto odir = dir / "other_annotations";

    initialize_directory_simple(cdir, "simple_list", "1.0");
    expect_error("'DATA_FRAME'"); 

    data_frame::mock(cdir, 4, {});
    initialize_directory_simple(odir, "data_frame", "1.0");
    expect_error("'SIMPLE_LIST'");

    simple_list::mock(odir);

    auto meta = takane::read_object_metadata(dir);
    takane::internal_compressed_list::validate_directory<false>(dir, "atomic_vector_list", "atomic_vector", meta, takane::Options());
}
