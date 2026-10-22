#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "takane/utils_bumpy_array.hpp"

#include "utils.h"
#include "mock_atomic_vector.h"
#include "mock_data_frame.h"
#include "mock_simple_list.h"

#include <string>
#include <filesystem>
#include <fstream>

template<bool satisfactory_>
void expect_bumpy_error(const std::filesystem::path& dir, const std::string& name, const std::string& msg) {
    auto meta = takane::read_object_metadata(dir);
    expect_error(
        msg,
        [&]() -> void {
            takane::validate_bumpy_array<satisfactory_>(dir, name, "atomic_vector", meta, {});
        }
    );
}

TEST(UtilsBumpyArray, DerivedOkay) {
    auto dir = define_test_path("utils_bumpy_array");

    {
        initialize_directory_simple(dir, "superfoobar", "1.0");
        H5::H5File handle(dir / "partitions.h5", H5F_ACC_TRUNC); 
        auto ghandle = handle.createGroup("superfoobar");
        add_hdf5_numeric_dataset(ghandle, "lengths", H5::PredType::NATIVE_UINT32, std::vector<int>{ 4, 3, 2, 1 });
        add_hdf5_numeric_dataset(ghandle, "dimensions", H5::PredType::NATIVE_UINT8, std::vector<int>{ 2, 2 });
        mock_atomic_vector(dir / "concatenated", AtomicVectorOptions(10));
    }

    auto meta = takane::read_object_metadata(dir);
    takane::validate_bumpy_array<false>(dir, "superfoobar", "atomic_vector", meta, {});
    EXPECT_EQ(takane::height_of_bumpy_array(dir, "superfoobar", meta, {}), 2);
    EXPECT_EQ(takane::dimensions_of_bumpy_array(dir, "superfoobar", meta, {}), (std::vector<size_t>{ 2, 2 }));
}

TEST(UtilsBumpyArray, SatisfiesOkay) {
    auto dir = define_test_path("utils_bumpy_array");

    {
        initialize_directory_simple(dir, "superfoobar", "1.0");
        H5::H5File handle(dir / "partitions.h5", H5F_ACC_TRUNC); 
        auto ghandle = handle.createGroup("superfoobar");
        add_hdf5_numeric_dataset(ghandle, "lengths", H5::PredType::NATIVE_UINT32, std::vector<int>{ 4, 3, 2, 1 });
        add_hdf5_numeric_dataset(ghandle, "dimensions", H5::PredType::NATIVE_UINT8, std::vector<int>{ 4, 1 });
        mock_data_frame(dir / "concatenated", DataFrameOptions(10, {}));
    }

    auto meta = takane::read_object_metadata(dir);
    takane::validate_bumpy_array<true>(dir, "superfoobar", "DATA_FRAME", meta, {});
    EXPECT_EQ(takane::height_of_bumpy_array(dir, "superfoobar", meta, {}), 4);
    EXPECT_EQ(takane::dimensions_of_bumpy_array(dir, "superfoobar", meta, {}), (std::vector<size_t>{ 4, 1 }));
}

TEST(UtilsBumpyArray, DenseOkay) {
    auto dir = define_test_path("utils_bumpy_array");

    // Trying an empty object.
    {
        initialize_directory_simple(dir, "superfoobar", "1.0");
        H5::H5File handle(dir / "partitions.h5", H5F_ACC_TRUNC); 
        auto ghandle = handle.createGroup("superfoobar");
        add_hdf5_numeric_dataset<int>(ghandle, "dimensions", H5::PredType::NATIVE_UINT8, { 0, 2 });
        ghandle.createDataSet("lengths", H5::PredType::NATIVE_UINT64, create_hdf5_dataspace(0));
        mock_atomic_vector(dir / "concatenated", AtomicVectorOptions(0, AtomicVectorType::INTEGER));
    }
    {
        auto meta = takane::read_object_metadata(dir);
        takane::validate_bumpy_array<false>(dir, "superfoobar", "atomic_vector", meta, {});
        EXPECT_EQ(takane::height_of_bumpy_array(dir, "superfoobar", meta, {}), 0);
        EXPECT_EQ(takane::dimensions_of_bumpy_array(dir, "superfoobar", meta, {}), (std::vector<size_t>{ 0, 2 }));
    }

    // Trying some higher-dimensional arrays.
    {
        initialize_directory_simple(dir, "superfoobar", "1.0");
        H5::H5File handle(dir / "partitions.h5", H5F_ACC_TRUNC); 
        auto ghandle = handle.createGroup("superfoobar");

        std::vector<int> dims{ 13, 9, 11 };
        std::size_t prod = 1;
        for (auto d : dims) {
            prod *= d; 
        }

        std::vector<int> lengths;
        std::size_t concat_length = 0;
        for (size_t i = 0; i < prod; ++i) {
            lengths.push_back(i % 4 + 1);
            concat_length += lengths.back();
        }

        add_hdf5_numeric_dataset(ghandle, "dimensions", H5::PredType::NATIVE_UINT8, dims);
        add_hdf5_numeric_dataset(ghandle, "lengths", H5::PredType::NATIVE_UINT32, lengths);
        mock_atomic_vector(dir / "concatenated", AtomicVectorOptions(concat_length, AtomicVectorType::INTEGER));
    }
    {
        auto meta = takane::read_object_metadata(dir);
        takane::validate_bumpy_array<false>(dir, "superfoobar", "atomic_vector", meta, {});
        EXPECT_EQ(takane::height_of_bumpy_array(dir, "superfoobar", meta, {}), 13);
        EXPECT_EQ(takane::dimensions_of_bumpy_array(dir, "superfoobar", meta, {}), (std::vector<size_t>{ 13, 9, 11 }));
    }
}

/***************************************/

TEST(UtilsBumpyArray, VersionError) {
    auto dir = define_test_path("utils_bumpy_array");

    {
        initialize_directory_simple(dir, "superfoobar", "2.0");
    }
    expect_bumpy_error<false>(dir, "superfoobar", "unsupported version");
}

TEST(UtilsBumpyArray, ConcatenatedError) {
    auto dir = define_test_path("utils_bumpy_array");

    {
        initialize_directory_simple(dir, "superfoobar", "1.0");
        initialize_directory_simple(dir / "concatenated", "foobar", "1.0");
    }
    expect_bumpy_error<false>(dir, "superfoobar", "derived from the 'atomic_vector'");
    expect_bumpy_error<true>(dir, "superfoobar", "satisfy the 'atomic_vector'");

    // Check that we actually validate the concatenated object.
    {
        initialize_directory_simple(dir, "superfoobar", "1.0");
        auto ghandle = mock_atomic_vector(dir / "concatenated", AtomicVectorOptions(5));
        ghandle.removeAttr("type");
        add_hdf5_string_attribute(ghandle, "type", "blah");
    }
    expect_bumpy_error<false>(dir, "superfoobar", "unsupported type 'blah'");
}

TEST(UtilsBumpyArray, DimensionsError) {
    auto dir = define_test_path("utils_bumpy_array");

    {
        initialize_directory_simple(dir, "superfoobar", "1.0");
        H5::H5File handle(dir / "partitions.h5", H5F_ACC_TRUNC); 
        auto ghandle = handle.createGroup("superfoobar");
        ghandle.createDataSet("dimensions", H5::PredType::NATIVE_INT8, create_hdf5_dataspace(2));
        mock_atomic_vector(dir / "concatenated", AtomicVectorOptions(10));
    }
    expect_bumpy_error<false>(dir, "superfoobar", "64-bit unsigned integer");

    {
        initialize_directory_simple(dir, "superfoobar", "1.0");
        H5::H5File handle(dir / "partitions.h5", H5F_ACC_TRUNC); 
        auto ghandle = handle.createGroup("superfoobar");
        ghandle.createDataSet("dimensions", H5::PredType::NATIVE_UINT8, H5S_SCALAR);
        mock_atomic_vector(dir / "concatenated", AtomicVectorOptions(10));
    }
    expect_bumpy_error<false>(dir, "superfoobar", "1-dimensional dataset");
}

TEST(UtilsBumpyArray, LengthsError) {
    auto dir = define_test_path("utils_bumpy_array");

    {
        initialize_directory_simple(dir, "superfoobar", "1.0");
        H5::H5File handle(dir / "partitions.h5", H5F_ACC_TRUNC); 
        auto ghandle = handle.createGroup("superfoobar");
        add_hdf5_numeric_dataset<int>(ghandle, "dimensions", H5::PredType::NATIVE_UINT8, { 3, 2 });
        ghandle.createDataSet("lengths", H5::PredType::NATIVE_INT8, create_hdf5_dataspace(6));
        mock_atomic_vector(dir / "concatenated", AtomicVectorOptions(10));
    }
    expect_bumpy_error<false>(dir, "superfoobar", "64-bit unsigned integer");

    {
        initialize_directory_simple(dir, "superfoobar", "1.0");
        H5::H5File handle(dir / "partitions.h5", H5F_ACC_TRUNC); 
        auto ghandle = handle.createGroup("superfoobar");
        add_hdf5_numeric_dataset<int>(ghandle, "dimensions", H5::PredType::NATIVE_UINT8, { 3, 2 });
        ghandle.createDataSet("lengths", H5::PredType::NATIVE_UINT8, H5S_SCALAR);
        mock_atomic_vector(dir / "concatenated", AtomicVectorOptions(10));
    }
    expect_bumpy_error<false>(dir, "superfoobar", "1-dimensional dataset");

    {
        initialize_directory_simple(dir, "superfoobar", "1.0");
        H5::H5File handle(dir / "partitions.h5", H5F_ACC_TRUNC); 
        auto ghandle = handle.createGroup("superfoobar");
        add_hdf5_numeric_dataset<int>(ghandle, "dimensions", H5::PredType::NATIVE_UINT8, { 3, 2 });
        add_hdf5_numeric_dataset<int>(ghandle, "lengths", H5::PredType::NATIVE_UINT8, { 1, 1, 1, 2, 2, 2 });
        mock_atomic_vector(dir / "concatenated", AtomicVectorOptions(10));
    }
    expect_bumpy_error<false>(dir, "superfoobar", "does not equal the height");

    {
        initialize_directory_simple(dir, "superfoobar", "1.0");
        H5::H5File handle(dir / "partitions.h5", H5F_ACC_TRUNC); 
        auto ghandle = handle.createGroup("superfoobar");
        add_hdf5_numeric_dataset<int>(ghandle, "dimensions", H5::PredType::NATIVE_UINT8, { 3, 2 });
        add_hdf5_numeric_dataset<int>(ghandle, "lengths", H5::PredType::NATIVE_UINT8, { 1, 1, 1, 2, 5 });
        mock_atomic_vector(dir / "concatenated", AtomicVectorOptions(10));
    }
    expect_bumpy_error<false>(dir, "superfoobar", "product of 'dimensions'");
}

/***************************************/

TEST(UtilsBumpyArray, SparseOkay) {
    auto dir = define_test_path("utils_bumpy_array");

    {
        initialize_directory_simple(dir, "superfoobar", "1.0");
        H5::H5File handle(dir / "partitions.h5", H5F_ACC_TRUNC); 
        auto ghandle = handle.createGroup("superfoobar");

        add_hdf5_numeric_dataset<int>(ghandle, "lengths", H5::PredType::NATIVE_UINT8, { 2, 1, 2, 2, 1, 1, 2 });
        add_hdf5_numeric_dataset<int>(ghandle, "dimensions", H5::PredType::NATIVE_UINT8, { 4, 3 });

        auto ihandle = ghandle.createGroup("indices");
        add_hdf5_numeric_dataset<int>(ihandle, "0", H5::PredType::NATIVE_UINT8, { 0, 2, 0, 3, 1, 2, 3 });
        add_hdf5_numeric_dataset<int>(ihandle, "1", H5::PredType::NATIVE_UINT8, { 0, 0, 1, 1, 2, 2, 2 });

        mock_atomic_vector(dir / "concatenated", AtomicVectorOptions(11, AtomicVectorType::INTEGER));
    }
    {
        auto meta = takane::read_object_metadata(dir);
        takane::validate_bumpy_array<false>(dir, "superfoobar", "atomic_vector", meta, {});
        EXPECT_EQ(takane::height_of_bumpy_array(dir, "superfoobar", meta, {}), 4);
        EXPECT_EQ(takane::dimensions_of_bumpy_array(dir, "superfoobar", meta, {}), (std::vector<std::size_t>{ 4, 3 }));
    }

    // Get some coverage on the empty case.
    {
        initialize_directory_simple(dir, "superfoobar", "1.0");
        H5::H5File handle(dir / "partitions.h5", H5F_ACC_TRUNC); 
        auto ghandle = handle.createGroup("superfoobar");
        add_hdf5_numeric_dataset<int>(ghandle, "lengths", H5::PredType::NATIVE_UINT8, {});
        add_hdf5_numeric_dataset<int>(ghandle, "dimensions", H5::PredType::NATIVE_UINT8, { 3, 4 });

        auto ihandle = ghandle.createGroup("indices");
        add_hdf5_numeric_dataset<int>(ihandle, "0", H5::PredType::NATIVE_UINT8, {});
        add_hdf5_numeric_dataset<int>(ihandle, "1", H5::PredType::NATIVE_UINT8, {});

        mock_atomic_vector(dir / "concatenated", AtomicVectorOptions(0, AtomicVectorType::INTEGER));
    }
    {
        auto meta = takane::read_object_metadata(dir);
        takane::validate_bumpy_array<false>(dir, "superfoobar", "atomic_vector", meta, {});
        EXPECT_EQ(takane::height_of_bumpy_array(dir, "superfoobar", meta, {}), 3);
        EXPECT_EQ(takane::dimensions_of_bumpy_array(dir, "superfoobar", meta, {}), (std::vector<std::size_t>{ 3, 4 }));
    }

    // Get some coverage for higher-dimensional arrays.
    {
        initialize_directory_simple(dir, "superfoobar", "1.0");
        H5::H5File handle(dir / "partitions.h5", H5F_ACC_TRUNC); 
        auto ghandle = handle.createGroup("superfoobar");

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

        add_hdf5_numeric_dataset(ghandle, "lengths", H5::PredType::NATIVE_UINT8, lengths);
        add_hdf5_numeric_dataset(ghandle, "dimensions", H5::PredType::NATIVE_UINT8, dims);

        auto ihandle = ghandle.createGroup("indices");
        add_hdf5_numeric_dataset(ihandle, "0", H5::PredType::NATIVE_UINT8, I);
        add_hdf5_numeric_dataset(ihandle, "1", H5::PredType::NATIVE_UINT8, J);
        add_hdf5_numeric_dataset(ihandle, "2", H5::PredType::NATIVE_UINT8, K);

        mock_atomic_vector(dir / "concatenated", AtomicVectorOptions(concat_length, AtomicVectorType::INTEGER));
    }
    {
        auto meta = takane::read_object_metadata(dir);
        takane::validate_bumpy_array<false>(dir, "superfoobar", "atomic_vector", meta, {});
        EXPECT_EQ(takane::height_of_bumpy_array(dir, "superfoobar", meta, {}), 11);
        EXPECT_EQ(takane::dimensions_of_bumpy_array(dir, "superfoobar", meta, {}), (std::vector<std::size_t>{ 11, 9, 13 }));
    }
}

TEST(UtilsBumpyArray, SparseError) {
    auto dir = define_test_path("utils_bumpy_array");

    auto initialize = [&]() -> H5::Group {
        initialize_directory_simple(dir, "superfoobar", "1.0");
        H5::H5File handle(dir / "partitions.h5", H5F_ACC_TRUNC); 
        auto ghandle = handle.createGroup("superfoobar");

        add_hdf5_numeric_dataset<int>(ghandle, "lengths", H5::PredType::NATIVE_UINT8, { 1, 2, 1, 2, 1, 2 });
        add_hdf5_numeric_dataset<int>(ghandle, "dimensions", H5::PredType::NATIVE_UINT8, { 4, 3 });
        mock_atomic_vector(dir / "concatenated", AtomicVectorOptions(9, AtomicVectorType::INTEGER));
        return ghandle;
    };

    {
        auto ghandle = initialize();
        auto ihandle = ghandle.createGroup("indices");
        ihandle.createDataSet("0", H5::PredType::NATIVE_INT8, create_hdf5_dataspace(6));
    }
    expect_bumpy_error<false>(dir, "superfoobar", "64-bit unsigned integer");

    {
        auto ghandle = initialize();
        auto ihandle = ghandle.createGroup("indices");
        ihandle.createDataSet("0", H5::PredType::NATIVE_UINT8, H5S_SCALAR);
    }
    expect_bumpy_error<false>(dir, "superfoobar", "1-dimensional dataset");

    {
        auto ghandle = initialize();
        auto ihandle = ghandle.createGroup("indices");
        ihandle.createDataSet("0", H5::PredType::NATIVE_UINT8, create_hdf5_dataspace(2));
    }
    expect_bumpy_error<false>(dir, "superfoobar", "same length as 'lengths'");

    // First position is out of range (faster changing).
    {
        auto ghandle = initialize();
        auto ihandle = ghandle.createGroup("indices");
        add_hdf5_numeric_dataset<int>(ihandle, "0", H5::PredType::NATIVE_UINT8, { 4, 2, 3, 1, 0, 3 });
        add_hdf5_numeric_dataset<int>(ihandle, "1", H5::PredType::NATIVE_UINT8, { 0, 0, 0, 1, 2, 2 });
    }
    expect_bumpy_error<false>(dir, "superfoobar", "less than the dimension extent");

    // First position is out of range (slower changing).
    {
        auto ghandle = initialize();
        auto ihandle = ghandle.createGroup("indices");
        add_hdf5_numeric_dataset<int>(ihandle, "0", H5::PredType::NATIVE_UINT8, { 0, 1, 2, 2, 3, 3 });
        add_hdf5_numeric_dataset<int>(ihandle, "1", H5::PredType::NATIVE_UINT8, { 3, 0, 0, 1, 1, 2 });
    }
    expect_bumpy_error<false>(dir, "superfoobar", "less than the dimension extent");

    // Other positions are out of range (faster changing).
    {
        auto ghandle = initialize();
        auto ihandle = ghandle.createGroup("indices");
        add_hdf5_numeric_dataset<int>(ihandle, "0", H5::PredType::NATIVE_UINT8, { 1, 2, 4, 1, 2, 3 });
        add_hdf5_numeric_dataset<int>(ihandle, "1", H5::PredType::NATIVE_UINT8, { 0, 0, 0, 2, 2, 2 });
    }
    expect_bumpy_error<false>(dir, "superfoobar", "less than the dimension extent");

    // Other positions are out of range (slower changing).
    {
        auto ghandle = initialize();
        auto ihandle = ghandle.createGroup("indices");
        add_hdf5_numeric_dataset<int>(ihandle, "0", H5::PredType::NATIVE_UINT8, { 0, 2, 3, 0, 1, 3 });
        add_hdf5_numeric_dataset<int>(ihandle, "1", H5::PredType::NATIVE_UINT8, { 1, 1, 1, 2, 2, 3 });
    }
    expect_bumpy_error<false>(dir, "superfoobar", "less than the dimension extent");

    // Positions are unsorted (faster changing).
    {
        auto ghandle = initialize();
        auto ihandle = ghandle.createGroup("indices");
        add_hdf5_numeric_dataset<int>(ihandle, "0", H5::PredType::NATIVE_UINT8, { 0, 2, 0, 0, 1, 3 });
        add_hdf5_numeric_dataset<int>(ihandle, "1", H5::PredType::NATIVE_UINT8, { 0, 1, 1, 2, 2, 3 });
    }
    expect_bumpy_error<false>(dir, "superfoobar", "strictly increasing");

    // Positions are unsorted (slower changing).
    {
        auto ghandle = initialize();
        auto ihandle = ghandle.createGroup("indices");
        add_hdf5_numeric_dataset<int>(ihandle, "0", H5::PredType::NATIVE_UINT8, { 1, 2, 3, 1, 2, 3 });
        add_hdf5_numeric_dataset<int>(ihandle, "1", H5::PredType::NATIVE_UINT8, { 0, 0, 2, 1, 2, 2 });
    }
    expect_bumpy_error<false>(dir, "superfoobar", "strictly increasing");

    // Duplicate positions.
    {
        auto ghandle = initialize();
        auto ihandle = ghandle.createGroup("indices");
        add_hdf5_numeric_dataset<int>(ihandle, "0", H5::PredType::NATIVE_UINT8, { 1, 2, 0, 0, 0, 3 });
        add_hdf5_numeric_dataset<int>(ihandle, "1", H5::PredType::NATIVE_UINT8, { 0, 0, 1, 1, 2, 2 });
    }
    expect_bumpy_error<false>(dir, "superfoobar", "duplicate");
}

/***************************************/

TEST(UtilsBumpyArray, NamesOkay) {
    auto dir = define_test_path("utils_bumpy_array");

    {
        initialize_directory_simple(dir, "superfoobar", "1.0");
        H5::H5File handle(dir / "partitions.h5", H5F_ACC_TRUNC); 
        auto ghandle = handle.createGroup("superfoobar");
        add_hdf5_numeric_dataset(ghandle, "lengths", H5::PredType::NATIVE_UINT32, std::vector<int>{ 2, 3, 2, 1, 1, 2 });
        add_hdf5_numeric_dataset(ghandle, "dimensions", H5::PredType::NATIVE_UINT8, std::vector<int>{ 2, 3 });
        mock_atomic_vector(dir / "concatenated", AtomicVectorOptions(11));
        auto nhandle = ghandle.createGroup("names");
        nhandle.createDataSet("0", H5::StrType(0, 5), create_hdf5_dataspace(2));
        nhandle.createDataSet("1", H5::StrType(0, 5), create_hdf5_dataspace(3));
    }

    auto meta = takane::read_object_metadata(dir);
    takane::validate_bumpy_array<false>(dir, "superfoobar", "atomic_vector", meta, {});
    EXPECT_EQ(takane::height_of_bumpy_array(dir, "superfoobar", meta, {}), 2);
    EXPECT_EQ(takane::dimensions_of_bumpy_array(dir, "superfoobar", meta, {}), (std::vector<size_t>{ 2, 3 }));
}

TEST(UtilsBumpyArray, NamesError) {
    auto dir = define_test_path("utils_bumpy_array");

    // Just confirm that we actually validate the names.
    {
        initialize_directory_simple(dir, "superfoobar", "1.0");
        H5::H5File handle(dir / "partitions.h5", H5F_ACC_TRUNC); 
        auto ghandle = handle.createGroup("superfoobar");
        add_hdf5_numeric_dataset(ghandle, "lengths", H5::PredType::NATIVE_UINT32, std::vector<int>{ 2, 3, 2, 1, 1, 2 });
        add_hdf5_numeric_dataset(ghandle, "dimensions", H5::PredType::NATIVE_UINT8, std::vector<int>{ 2, 3 });
        mock_atomic_vector(dir / "concatenated", AtomicVectorOptions(11));
        auto nhandle = ghandle.createGroup("names");
        nhandle.createDataSet("1", H5::StrType(0, 5), create_hdf5_dataspace(2));
    }

    expect_bumpy_error<false>(dir, "superfoobar", "same length as the extent");
}
