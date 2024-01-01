#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "takane/utils_bumpy_array.hpp"
#include "utils.h"
#include "atomic_vector.h"
#include "data_frame.h"
#include "simple_list.h"

#include <string>
#include <filesystem>
#include <fstream>

struct BumpyArrayUtilsTest : public::testing::Test {
    BumpyArrayUtilsTest() {
        dir = "TEST_bumpy_atomic_array";
        name = "bumpy_atomic_array";
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
                takane::internal_bumpy_array::validate_directory<satisfactory>(dir, name, "atomic_vector", meta, takane::Options());
            } catch (std::exception& e) {
                EXPECT_THAT(e.what(), ::testing::HasSubstr(msg));
                throw;
            }
        });
    }
};

TEST_F(BumpyArrayUtilsTest, Basic) {
    initialize_directory_simple(dir, name, "2.0");
    expect_error("unsupported version string");

    {
        auto handle = initialize();
        auto ghandle = handle.createGroup(name);
        hdf5_utils::spawn_numeric_data<int>(ghandle, "lengths", H5::PredType::NATIVE_UINT32, { 4, 3, 2, 1 });
        hdf5_utils::spawn_numeric_data<int>(ghandle, "dimensions", H5::PredType::NATIVE_UINT8, { 2, 2 });
        initialize_directory_simple(dir / "concatenated", "foobar", "1.0");
    }
    expect_error("should contain an 'atomic_vector'");
    expect_error<true>("'atomic_vector' interface");

    initialize_directory_simple(dir / "concatenated", "atomic_vector", "1.0");
    expect_error("failed to validate the 'concatenated'");

    {
        atomic_vector::mock(dir / "concatenated", 10, atomic_vector::Type::INTEGER);
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        ghandle.unlink("dimensions");
        hdf5_utils::spawn_numeric_data<int>(ghandle, "dimensions", H5::PredType::NATIVE_INT8, { 2, 2 });
    }
    expect_error("64-bit unsigned integer");

    // Success at last!
    {
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        ghandle.unlink("dimensions");
        hdf5_utils::spawn_numeric_data<int>(ghandle, "dimensions", H5::PredType::NATIVE_UINT8, { 2, 2 });
    }
    auto meta = takane::read_object_metadata(dir);
    takane::internal_bumpy_array::validate_directory<false>(dir, "bumpy_atomic_array", "atomic_vector", meta, takane::Options());

    EXPECT_EQ(takane::internal_bumpy_array::height(dir, name, meta, takane::Options()), 2);
    auto dims = takane::internal_bumpy_array::dimensions(dir, name, meta, takane::Options());
    std::vector<size_t> expected { 2, 2 };
    EXPECT_EQ(dims, expected);
}

TEST_F(BumpyArrayUtilsTest, Lengths) {
    {
        auto handle = initialize();
        auto ghandle = handle.createGroup(name);
        hdf5_utils::spawn_numeric_data<int>(ghandle, "lengths", H5::PredType::NATIVE_INT32, { 4, 3, 2, 1 });
        hdf5_utils::spawn_numeric_data<int>(ghandle, "dimensions", H5::PredType::NATIVE_UINT8, { 2, 2 });
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

TEST_F(BumpyArrayUtilsTest, Dense) {
    {
        auto handle = initialize();
        auto ghandle = handle.createGroup(name);
        hdf5_utils::spawn_numeric_data<int>(ghandle, "lengths", H5::PredType::NATIVE_UINT8, { 4, 3, 2, 1 });
        hdf5_utils::spawn_numeric_data<int>(ghandle, "dimensions", H5::PredType::NATIVE_UINT8, { 3, 2 });
        atomic_vector::mock(dir / "concatenated", 10, atomic_vector::Type::INTEGER);
    }
    expect_error("length of 'lengths' should equal the product of 'dimensions'");

    // Empty case.
    {
        auto handle = initialize();
        auto ghandle = handle.createGroup(name);
        hdf5_utils::spawn_numeric_data<int>(ghandle, "lengths", H5::PredType::NATIVE_UINT8, {});
        hdf5_utils::spawn_numeric_data<int>(ghandle, "dimensions", H5::PredType::NATIVE_UINT8, { 0, 2 });
        atomic_vector::mock(dir / "concatenated", 0, atomic_vector::Type::INTEGER);
    }
    {
        auto meta = takane::read_object_metadata(dir);
        takane::internal_bumpy_array::validate_directory<false>(dir, "bumpy_atomic_array", "atomic_vector", meta, takane::Options());
        EXPECT_EQ(takane::internal_bumpy_array::height(dir, name, meta, takane::Options()), 0);
    }

    // Higher-dimensional arrays.
    {
        auto handle = initialize();
        auto ghandle = handle.createGroup(name);

        std::vector<int> dims{ 13, 9, 11 };
        size_t prod = 1;
        for (auto d : dims) {
            prod *= d; 
        }

        std::vector<int> lengths;
        size_t concat_length = 0;
        for (size_t i = 0; i < prod; ++i) {
            lengths.push_back(i % 4 + 1);
            concat_length += lengths.back();
        }

        hdf5_utils::spawn_numeric_data<int>(ghandle, "lengths", H5::PredType::NATIVE_UINT8, lengths);
        hdf5_utils::spawn_numeric_data<int>(ghandle, "dimensions", H5::PredType::NATIVE_UINT8, dims);
        atomic_vector::mock(dir / "concatenated", concat_length, atomic_vector::Type::INTEGER);
    }
    {
        auto meta = takane::read_object_metadata(dir);
        takane::internal_bumpy_array::validate_directory<false>(dir, "bumpy_atomic_array", "atomic_vector", meta, takane::Options());
        EXPECT_EQ(takane::internal_bumpy_array::height(dir, name, meta, takane::Options()), 13);
    }
}

TEST_F(BumpyArrayUtilsTest, Sparse) {
    {
        auto handle = initialize();
        auto ghandle = handle.createGroup(name);
        hdf5_utils::spawn_numeric_data<int>(ghandle, "lengths", H5::PredType::NATIVE_UINT8, { 2, 1, 2, 2, 1, 1, 2 });
        hdf5_utils::spawn_numeric_data<int>(ghandle, "dimensions", H5::PredType::NATIVE_UINT8, { 4, 3 });

        auto ihandle = ghandle.createGroup("indices");
        hdf5_utils::spawn_numeric_data<int>(ihandle, "0", H5::PredType::NATIVE_UINT8, { 0, 2, 0, 3, 1, 2, 3 });
        hdf5_utils::spawn_numeric_data<int>(ihandle, "1", H5::PredType::NATIVE_UINT8, { 0, 0, 1, 1, 2, 2, 2 });

        atomic_vector::mock(dir / "concatenated", 11, atomic_vector::Type::INTEGER);
    }

    auto meta = takane::read_object_metadata(dir);
    takane::internal_bumpy_array::validate_directory<false>(dir, "bumpy_atomic_array", "atomic_vector", meta, takane::Options());
    EXPECT_EQ(takane::internal_bumpy_array::height(dir, name, meta, takane::Options()), 4);
    auto dims = takane::internal_bumpy_array::dimensions(dir, name, meta, takane::Options());
    std::vector<size_t> expected { 4, 3 };
    EXPECT_EQ(dims, expected);

    // Mismatching lengths.
    {
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        auto ihandle = ghandle.openGroup("indices");
        ihandle.unlink("0");
        hdf5_utils::spawn_numeric_data<int>(ihandle, "0", H5::PredType::NATIVE_UINT8, { 0, 2 });
    }
    expect_error("same length as 'lengths'");

    // Wrong type.
    {
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        auto ihandle = ghandle.openGroup("indices");
        ihandle.unlink("0");
        hdf5_utils::spawn_numeric_data<int>(ihandle, "0", H5::PredType::NATIVE_INT, { 0, 2, 0, 3, 1, 2, 3 });
    }
    expect_error("64-bit unsigned integer");

    // Out of range.
    {
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        auto ihandle = ghandle.openGroup("indices");
        ihandle.unlink("0");
        hdf5_utils::spawn_numeric_data<int>(ihandle, "0", H5::PredType::NATIVE_UINT8, { 4, 2, 0, 3, 1, 2, 3 });
    }
    expect_error("less than the corresponding dimension extent");

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        auto ihandle = ghandle.openGroup("indices");
        ihandle.unlink("0");
        hdf5_utils::spawn_numeric_data<int>(ihandle, "0", H5::PredType::NATIVE_UINT8, { 0, 4, 0, 3, 1, 2, 3 });
    }
    expect_error("less than the corresponding dimension extent");

    // Not strictly increasing.
    {
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        auto ihandle = ghandle.openGroup("indices");
        ihandle.unlink("0");
        hdf5_utils::spawn_numeric_data<int>(ihandle, "0", H5::PredType::NATIVE_UINT8, { 2, 0, 0, 3, 1, 2, 3 });
    }
    expect_error("should be strictly increasing");

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        auto ihandle = ghandle.openGroup("indices");
        ihandle.unlink("0");
        hdf5_utils::spawn_numeric_data<int>(ihandle, "0", H5::PredType::NATIVE_UINT8, { 1, 1, 0, 3, 1, 2, 3 });
    }
    expect_error("duplicate coordinates");

    // Get some coverage on the empty case.
    {
        auto handle = initialize();
        auto ghandle = handle.createGroup(name);
        hdf5_utils::spawn_numeric_data<int>(ghandle, "lengths", H5::PredType::NATIVE_UINT8, {});
        hdf5_utils::spawn_numeric_data<int>(ghandle, "dimensions", H5::PredType::NATIVE_UINT8, { 4, 3 });

        auto ihandle = ghandle.createGroup("indices");
        hdf5_utils::spawn_numeric_data<int>(ihandle, "0", H5::PredType::NATIVE_UINT8, {});
        hdf5_utils::spawn_numeric_data<int>(ihandle, "1", H5::PredType::NATIVE_UINT8, {});

        atomic_vector::mock(dir / "concatenated", 0, atomic_vector::Type::INTEGER);
    }
    {
        auto meta = takane::read_object_metadata(dir);
        takane::internal_bumpy_array::validate_directory<false>(dir, "bumpy_atomic_array", "atomic_vector", meta, takane::Options());
        EXPECT_EQ(takane::internal_bumpy_array::height(dir, name, meta, takane::Options()), 4);
    }

    // Get some coverage for higher-dimensional arrays.
    {
        auto handle = initialize();
        auto ghandle = handle.createGroup(name);

        std::vector<int> lengths;
        std::vector<int> I, J, K;
        std::vector<int> dims { 11, 9, 13 };
        size_t concat_length = 0, counter = 0;
        for (int k = 0; k < dims[2]; ++k) {
            for (int j = 0; j < dims[1]; ++j) {
                for (int i = 0; i < dims[0]; ++i, ++counter) {
                    if (counter % 7 == 0) { // only keep points at a particular interval to keep things exciting.
                        I.push_back(i);
                        J.push_back(j);
                        K.push_back(k);
                        lengths.push_back(counter % 4 + 1);
                        concat_length += lengths.back();
                    }
                }
            }
        }

        hdf5_utils::spawn_numeric_data<int>(ghandle, "lengths", H5::PredType::NATIVE_UINT8, lengths);
        hdf5_utils::spawn_numeric_data<int>(ghandle, "dimensions", H5::PredType::NATIVE_UINT8, dims);

        auto ihandle = ghandle.createGroup("indices");
        hdf5_utils::spawn_numeric_data<int>(ihandle, "0", H5::PredType::NATIVE_UINT8, I);
        hdf5_utils::spawn_numeric_data<int>(ihandle, "1", H5::PredType::NATIVE_UINT8, J);
        hdf5_utils::spawn_numeric_data<int>(ihandle, "2", H5::PredType::NATIVE_UINT8, K);

        atomic_vector::mock(dir / "concatenated", concat_length, atomic_vector::Type::INTEGER);
    }
    {
        auto meta = takane::read_object_metadata(dir);
        takane::internal_bumpy_array::validate_directory<false>(dir, "bumpy_atomic_array", "atomic_vector", meta, takane::Options());
        EXPECT_EQ(takane::internal_bumpy_array::height(dir, name, meta, takane::Options()), 11);
    }
}

TEST_F(BumpyArrayUtilsTest, Names) {
    {
        auto handle = initialize();
        auto ghandle = handle.createGroup(name);
        hdf5_utils::spawn_numeric_data<int>(ghandle, "lengths", H5::PredType::NATIVE_UINT32, { 1, 2, 3, 4, 4, 3, 2, 1 });
        hdf5_utils::spawn_numeric_data<int>(ghandle, "dimensions", H5::PredType::NATIVE_UINT8, { 2, 4 });
        atomic_vector::mock(dir / "concatenated", 20, atomic_vector::Type::NUMBER);

        auto nhandle = ghandle.createGroup("names");
        hdf5_utils::spawn_string_data(nhandle, "0", H5T_VARIABLE, { "Frieren", "Fern" });
        hdf5_utils::spawn_string_data(nhandle, "1", H5T_VARIABLE, { "Aaron", "Charlie", "Echo", "Fooblewooble" });
    }
    auto meta = takane::read_object_metadata(dir);
    takane::internal_bumpy_array::validate_directory<false>(dir, "bumpy_atomic_array", "atomic_vector", meta, takane::Options());

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        auto nhandle = ghandle.openGroup("names");
        nhandle.unlink("0");
        hdf5_utils::spawn_string_data(nhandle, "0", H5T_VARIABLE, { "Aaron" });
    }
    expect_error("same length");
}

TEST_F(BumpyArrayUtilsTest, Metadata) {
    {
        auto handle = initialize();
        auto ghandle = handle.createGroup(name);
        hdf5_utils::spawn_numeric_data<int>(ghandle, "lengths", H5::PredType::NATIVE_UINT32, { 4, 3, 2, 1 });
        hdf5_utils::spawn_numeric_data<int>(ghandle, "dimensions", H5::PredType::NATIVE_UINT8, { 2, 2 });
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
    takane::internal_bumpy_array::validate_directory<false>(dir, "bumpy_atomic_array", "atomic_vector", meta, takane::Options());
}
