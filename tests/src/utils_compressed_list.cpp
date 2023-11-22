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

struct CompressedListListTest : public::testing::Test {
    CompressedListListTest() {
        dir = "TEST_atomic_vector_list";
        name = "atomic_vector_list";
    }

    std::filesystem::path dir;
    std::string name;

    H5::H5File initialize() {
        initialize_directory(dir, name);
        return H5::H5File(dir / "partitions.h5", H5F_ACC_TRUNC);
    }

    H5::H5File reopen() {
        return H5::H5File(dir / "partitions.h5", H5F_ACC_RDWR);
    }

    template<bool satisfactory = false>
    void expect_error(const std::string& msg) {
        EXPECT_ANY_THROW({
            try {
                takane::internal_compressed_list::validate_directory<satisfactory>(dir, name, "atomic_vector", takane::Options());
            } catch (std::exception& e) {
                EXPECT_THAT(e.what(), ::testing::HasSubstr(msg));
                throw;
            }
        });
    }
};

TEST_F(CompressedListListTest, Basic) {
    {
        auto handle = initialize();
        auto ghandle = handle.createGroup(name);
        hdf5_utils::attach_attribute(ghandle, "version", "2.0");
    }
    expect_error("unsupported version string");

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        ghandle.removeAttr("version");
        hdf5_utils::attach_attribute(ghandle, "version", "1.0");
        hdf5_utils::spawn_numeric_data<int>(ghandle, "lengths", H5::PredType::NATIVE_UINT32, { 4, 3, 2, 1 });
        initialize_directory(dir / "concatenated", "foobar");
    }
    expect_error("should contain an 'atomic_vector'");
    expect_error<true>("'atomic_vector' interface");

    {
        initialize_directory(dir / "concatenated", "atomic_vector");
    }
    expect_error("failed to validate the 'concatenated'");

    // Success at last!
    {
        atomic_vector::mock(dir / "concatenated", 10, atomic_vector::Type::INTEGER);
    }
    takane::internal_compressed_list::validate_directory<false>(dir, "atomic_vector_list", "atomic_vector", takane::Options());
    EXPECT_EQ(takane::internal_compressed_list::height(dir, name, takane::Options()), 4);
}

TEST_F(CompressedListListTest, Lengths) {
    {
        auto handle = initialize();
        auto ghandle = handle.createGroup(name);
        hdf5_utils::attach_attribute(ghandle, "version", "1.0");
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

TEST_F(CompressedListListTest, Names) {
    {
        initialize_directory(dir, name);
        auto handle = H5::H5File(dir / "partitions.h5", H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup(name);
        hdf5_utils::attach_attribute(ghandle, "version", "1.0");
        hdf5_utils::spawn_numeric_data<int>(ghandle, "lengths", H5::PredType::NATIVE_UINT32, { 4, 3, 2, 1 });
        atomic_vector::mock(dir / "concatenated", 10, atomic_vector::Type::NUMBER);

        hdf5_utils::spawn_string_data(ghandle, "names", H5T_VARIABLE, { "Aaron", "Charlie", "Echo", "Fooblewooble" });
    }
    takane::internal_compressed_list::validate_directory<false>(dir, "atomic_vector_list", "atomic_vector", takane::Options());

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        ghandle.unlink("names");
        hdf5_utils::spawn_string_data(ghandle, "names", H5T_VARIABLE, { "Aaron" });
    }
    expect_error("same length");
}

TEST_F(CompressedListListTest, Metadata) {
    {
        initialize_directory(dir, name);
        auto handle = H5::H5File(dir / "partitions.h5", H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup(name);
        hdf5_utils::attach_attribute(ghandle, "version", "1.0");
        hdf5_utils::spawn_numeric_data<int>(ghandle, "lengths", H5::PredType::NATIVE_UINT32, { 4, 3, 2, 1 });
        atomic_vector::mock(dir / "concatenated", 10, atomic_vector::Type::BOOLEAN);
    }

    auto cdir = dir / "element_annotations";
    auto odir = dir / "other_annotations";

    initialize_directory(cdir, "simple_list");
    expect_error("'DATA_FRAME'"); 

    data_frame::mock(cdir, 4, false, {});
    initialize_directory(odir, "data_frame");
    expect_error("'SIMPLE_LIST'");

    initialize_directory(odir, "simple_list");
    simple_list::mock(odir);
    takane::internal_compressed_list::validate_directory<false>(dir, "atomic_vector_list", "atomic_vector", takane::Options());
}
