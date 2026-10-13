#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "mock_compressed_sparse_matrix.h"
#include "utils.h"

#include <numeric>
#include <string>
#include <vector>
#include <random>

TEST(CompressedSparseMatrix, Okay) {
    auto dir = define_test_path("compressed_sparse_matrix");

    {
        mock_compressed_sparse_matrix(dir, CompressedSparseMatrixOptions(299, 121));
    }
    {
        test_validate(dir);
        EXPECT_EQ(test_height(dir), 299);
        EXPECT_EQ(test_dimensions(dir), (std::vector<std::size_t>{ 299, 121 }));
    }

    // CSR.
    {
        CompressedSparseMatrixOptions opt(182, 107);
        opt.column_sparse = false;
        mock_compressed_sparse_matrix(dir, opt);
    }
    {
        test_validate(dir);
        EXPECT_EQ(test_height(dir), 182);
        EXPECT_EQ(test_dimensions(dir), (std::vector<std::size_t>{ 182, 107 }));
    }

    // Double-precision.
    {
        CompressedSparseMatrixOptions opt(82, 154);
        opt.type = CompressedSparseMatrixType::NUMBER;
        mock_compressed_sparse_matrix(dir, opt);
    }
    {
        test_validate(dir);
        EXPECT_EQ(test_height(dir), 82);
        EXPECT_EQ(test_dimensions(dir), (std::vector<std::size_t>{ 82, 154 }));
    }

    // Boolean.
    {
        CompressedSparseMatrixOptions opt(32, 321);
        opt.type = CompressedSparseMatrixType::BOOLEAN;
        mock_compressed_sparse_matrix(dir, opt);
    }
    {
        test_validate(dir);
        EXPECT_EQ(test_height(dir), 32);
        EXPECT_EQ(test_dimensions(dir), (std::vector<std::size_t>{ 32, 321 }));
    }
}

TEST(CompressedSparseMatrix, ExtremeOkay) {
    auto dir = define_test_path("compressed_sparse_matrix");

    // All columns empty.
    {
        CompressedSparseMatrixOptions opt(20, 30);
        opt.density = 0;
        mock_compressed_sparse_matrix(dir, opt);
    }
    {
        test_validate(dir);
        EXPECT_EQ(test_height(dir), 20);
        EXPECT_EQ(test_dimensions(dir), (std::vector<std::size_t>{ 20, 30 }));
    }

    // All columns full.
    {
        CompressedSparseMatrixOptions opt(25, 35);
        opt.density = 1;
        mock_compressed_sparse_matrix(dir, opt);
    }
    {
        test_validate(dir);
        EXPECT_EQ(test_height(dir), 25);
        EXPECT_EQ(test_dimensions(dir), (std::vector<std::size_t>{ 25, 35 }));
    }
}

/**********************************/

TEST(CompressedSparseMatrix, VersionError) {
    auto dir = define_test_path("compressed_sparse_matrix");

    {
        initialize_directory_simple(dir, "compressed_sparse_matrix", "2.0");
    }
    expect_validation_error(dir, "unsupported version");
}

TEST(CompressedSparseMatrix, TypeError) {
    auto dir = define_test_path("compressed_sparse_matrix");

    // Test that the type attribute's type/shape are actually validated.
    {
        auto ghandle = mock_compressed_sparse_matrix(dir, CompressedSparseMatrixOptions(20, 30));
        ghandle.removeAttr("type");
        ghandle.createAttribute("type", H5::PredType::NATIVE_INT, H5S_SCALAR); 
    }
    expect_validation_error(dir, "UTF-8 encoded string");

    {
        auto ghandle = mock_compressed_sparse_matrix(dir, CompressedSparseMatrixOptions(20, 30));
        ghandle.removeAttr("type");
        add_hdf5_string_attribute(ghandle, "type", "foobar");
    }
    expect_validation_error(dir, "unknown matrix type");
}

TEST(CompressedSparseMatrix, LayoutError) {
    auto dir = define_test_path("compressed_sparse_matrix");

    // Test that the layout attribute's type/shape are actually validated.
    {
        auto ghandle = mock_compressed_sparse_matrix(dir, CompressedSparseMatrixOptions(20, 30));
        ghandle.removeAttr("layout");
        ghandle.createAttribute("layout", H5::PredType::NATIVE_INT, H5S_SCALAR); 
    }
    expect_validation_error(dir, "UTF-8 encoded string");

    {
        auto ghandle = mock_compressed_sparse_matrix(dir, CompressedSparseMatrixOptions(20, 30));
        ghandle.removeAttr("layout");
        add_hdf5_string_attribute(ghandle, "layout", "fooobar");
    }
    expect_validation_error(dir, "'layout' should be either");
}

TEST(CompressedSparseMatrix, ShapeError) {
    auto dir = define_test_path("compressed_sparse_matrix");

    {
        auto ghandle = mock_compressed_sparse_matrix(dir, CompressedSparseMatrixOptions(20, 30));
        ghandle.unlink("shape");
        ghandle.createDataSet("shape", H5::PredType::NATIVE_INT32, create_hdf5_dataspace(2));
    }
    expect_validation_error(dir, "64-bit unsigned integer");

    {
        auto ghandle = mock_compressed_sparse_matrix(dir, CompressedSparseMatrixOptions(20, 30));
        ghandle.unlink("shape");
        ghandle.createDataSet("shape", H5::PredType::NATIVE_UINT32, H5S_SCALAR);
    }
    expect_validation_error(dir, "1-dimensional");


    {
        auto ghandle = mock_compressed_sparse_matrix(dir, CompressedSparseMatrixOptions(20, 30));
        ghandle.unlink("shape");
        ghandle.createDataSet("shape", H5::PredType::NATIVE_UINT64, create_hdf5_dataspace(3));
    }
    expect_validation_error(dir, "length 2");
}

/**********************************/

TEST(CompressedSparseMatrix, DataError) {
    auto dir = define_test_path("compressed_sparse_matrix");

    {
        auto ghandle = mock_compressed_sparse_matrix(dir, CompressedSparseMatrixOptions(20, 30));
        ghandle.unlink("data");
        ghandle.createDataSet("data", H5::PredType::NATIVE_INT32, H5S_SCALAR);
    }
    expect_validation_error(dir, "1-dimensional");
}

TEST(CompressedSparseMatrix, IntegerError) {
    auto dir = define_test_path("compressed_sparse_matrix");

    {
        CompressedSparseMatrixOptions opt(20, 30);
        opt.type = CompressedSparseMatrixType::NUMBER;
        auto ghandle = mock_compressed_sparse_matrix(dir, opt);
        ghandle.removeAttr("type");
        add_hdf5_string_attribute(ghandle, "type", "integer");
    }
    expect_validation_error(dir, "32-bit signed integer");
}

TEST(CompressedSparseMatrix, BooleanError) {
    auto dir = define_test_path("compressed_sparse_matrix");

    {
        CompressedSparseMatrixOptions opt(20, 30);
        opt.type = CompressedSparseMatrixType::NUMBER;
        auto ghandle = mock_compressed_sparse_matrix(dir, opt);
        ghandle.removeAttr("type");
        add_hdf5_string_attribute(ghandle, "type", "boolean");
    }
    expect_validation_error(dir, "32-bit signed integer");
}

TEST(CompressedSparseMatrix, NumberError) {
    auto dir = define_test_path("compressed_sparse_matrix");

    {
        CompressedSparseMatrixOptions opt(20, 30);
        opt.type = CompressedSparseMatrixType::NUMBER;
        auto ghandle = mock_compressed_sparse_matrix(dir, opt);

        hsize_t len;
        {
            auto dhandle = ghandle.openDataSet("data");
            dhandle.getSpace().getSimpleExtentDims(&len);
        }
        ghandle.unlink("data");
        ghandle.createDataSet("data", H5::PredType::NATIVE_INT64, create_hdf5_dataspace(len));
    }
    expect_validation_error(dir, "64-bit float");
}

TEST(CompressedSparseMatrix, MissingOkay) {
    auto dir = define_test_path("compressed_sparse_matrix");

    {
        auto ghandle = mock_compressed_sparse_matrix(dir, CompressedSparseMatrixOptions(22, 35));
        auto dhandle = ghandle.openDataSet("data");
        dhandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_INT32, H5S_SCALAR);
    }

    test_validate(dir);
    EXPECT_EQ(test_height(dir), 22);
    std::vector<std::size_t> expected_dims { 22, 35 };
    EXPECT_EQ(test_dimensions(dir), expected_dims);
}

TEST(CompressedSparseMatrix, MissingError) {
    auto dir = define_test_path("compressed_sparse_matrix");

    // Test that the missing placeholder is actually validated.
    {
        auto ghandle = mock_compressed_sparse_matrix(dir, CompressedSparseMatrixOptions(22, 35));
        auto dhandle = ghandle.openDataSet("data");
        dhandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_UINT8, H5S_SCALAR);
    }
    expect_validation_error(dir, "same datatype");
}

/**********************************/

TEST(CompressedSparseMatrix, IndptrSimpleError) {
    auto dir = define_test_path("compressed_sparse_matrix");

    {
        int NC = 35;
        auto ghandle = mock_compressed_sparse_matrix(dir, CompressedSparseMatrixOptions(51, NC));
        ghandle.unlink("indptr");
        ghandle.createDataSet("indptr", H5::PredType::NATIVE_INT32, create_hdf5_dataspace(NC + 1));
    }
    expect_validation_error(dir, "64-bit unsigned integer");

    {
        auto ghandle = mock_compressed_sparse_matrix(dir, CompressedSparseMatrixOptions(51, 14));
        ghandle.unlink("indptr");
        ghandle.createDataSet("indptr", H5::PredType::NATIVE_UINT32, H5S_SCALAR);
    }
    expect_validation_error(dir, "1-dimensional");

    {
        int NC = 14;
        auto ghandle = mock_compressed_sparse_matrix(dir, CompressedSparseMatrixOptions(121, NC));
        ghandle.unlink("indptr");
        ghandle.createDataSet("indptr", H5::PredType::NATIVE_UINT32, create_hdf5_dataspace(NC));
    }
    expect_validation_error(dir, "number of columns plus 1");

    {
        CompressedSparseMatrixOptions options(55, 23);
        options.column_sparse = false;
        auto ghandle = mock_compressed_sparse_matrix(dir, options);
        ghandle.unlink("indptr");
        ghandle.createDataSet("indptr", H5::PredType::NATIVE_UINT32, create_hdf5_dataspace(100));
    }
    expect_validation_error(dir, "number of rows plus 1");
}

class CompressedSparseMatrixIndptrErrorTest : public ::testing::TestWithParam<bool> {};

TEST_P(CompressedSparseMatrixIndptrErrorTest, Hard) {
    auto dir = define_test_path("compressed_sparse_matrix");

    std::vector<std::size_t> dims{ 5, 8 };
    std::vector<double> data(8, 1.2);
    std::vector<int> indices{ 1, 1, 1, 4, 2, 1, 2, 3 };
    std::vector<int> ptrs{ 0, 1, 3, 4, 5, 6, 8, 8, 8 };

    CompressedSparseMatrixBasicOptions options;
    options.column_sparse = GetParam();
    if (!options.column_sparse) {
        std::reverse(dims.begin(), dims.end());
    }

    {
        initialize_directory_simple(dir, "compressed_sparse_matrix", "1.0");
        H5::H5File handle(dir / "matrix.h5", H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup("compressed_sparse_matrix");
        auto ptrcopy = ptrs;
        ptrcopy[0] = 1;
        mock_compressed_sparse_matrix(ghandle, dims, data, indices, ptrcopy, options);
    }
    expect_validation_error(dir, "should be zero");

    {
        initialize_directory_simple(dir, "compressed_sparse_matrix", "1.0");
        H5::H5File handle(dir / "matrix.h5", H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup("compressed_sparse_matrix");
        auto ptrcopy = ptrs;
        ptrcopy.back() += 1;
        mock_compressed_sparse_matrix(ghandle, dims, data, indices, ptrcopy, options);
    }
    expect_validation_error(dir, "equal the number of non-zero elements");

    {
        initialize_directory_simple(dir, "compressed_sparse_matrix", "1.0");
        H5::H5File handle(dir / "matrix.h5", H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup("compressed_sparse_matrix");
        auto ptrcopy = ptrs;
        std::reverse(ptrcopy.begin() + 1, ptrcopy.end() - 1); 
        mock_compressed_sparse_matrix(ghandle, dims, data, indices, ptrcopy, options);
    }
    expect_validation_error(dir, "should be sorted");
}

INSTANTIATE_TEST_SUITE_P(
    CompressedSparseMatrix,
    CompressedSparseMatrixIndptrErrorTest,
    ::testing::Values(true, false)
);

/**********************************/

TEST(CompressedSparseMatrix, IndicesSimpleError) {
    auto dir = define_test_path("compressed_sparse_matrix");

    {
        auto ghandle = mock_compressed_sparse_matrix(dir, CompressedSparseMatrixOptions(51, 74));
        ghandle.unlink("indices");
        ghandle.createDataSet("indices", H5::PredType::NATIVE_INT32, create_hdf5_dataspace(100));
    }
    expect_validation_error(dir, "64-bit unsigned integer");

    {
        auto ghandle = mock_compressed_sparse_matrix(dir, CompressedSparseMatrixOptions(51, 74));
        ghandle.unlink("indices");
        ghandle.createDataSet("indices", H5::PredType::NATIVE_UINT32, H5S_SCALAR);
    }
    expect_validation_error(dir, "1-dimensional");

    {
        auto ghandle = mock_compressed_sparse_matrix(dir, CompressedSparseMatrixOptions(51, 74));
        hsize_t len;
        {
            auto ihandle = ghandle.openDataSet("indices");
            ihandle.getSpace().getSimpleExtentDims(&len);
        }
        ghandle.unlink("indices");
        ghandle.createDataSet("indices", H5::PredType::NATIVE_UINT32, create_hdf5_dataspace(len + 1));
    }
    expect_validation_error(dir, "equal to the number of non-zero");
}

class CompressedSparseMatrixIndicesErrorTest : public ::testing::TestWithParam<bool> {};

TEST_P(CompressedSparseMatrixIndicesErrorTest, Hard) {
    auto dir = define_test_path("compressed_sparse_matrix");

    std::vector<std::size_t> dims{ 15, 6 };
    std::vector<double> data(18, -2.2);
    std::vector<int> indices{ 
        0, 3, 11,
        4, 5, 8, 10,
        2, 5, 9, 13,
        10, 13,
        8,
        0, 4, 10, 14
    };
    std::vector<int> ptrs{ 0, 3, 7, 11, 13, 14, 18 };

    CompressedSparseMatrixBasicOptions options;
    options.column_sparse = GetParam();
    if (!options.column_sparse) {
        std::reverse(dims.begin(), dims.end());
    }

    // Check what happens if we insert an unsorted value at the non-last position of each column/row.
    for (auto pos : std::vector<int>{1, 5, 9, 11, 16}) {
        {
            initialize_directory_simple(dir, "compressed_sparse_matrix", "1.0");
            H5::H5File handle(dir / "matrix.h5", H5F_ACC_TRUNC);
            auto ghandle = handle.createGroup("compressed_sparse_matrix");
            auto icopy = indices;
            icopy[pos] = 15;
            mock_compressed_sparse_matrix(ghandle, dims, data, icopy, ptrs, options);
        }
        expect_validation_error(dir, "should be strictly increasing");

        takane::Options opt;
        opt.hdf5_buffer_size = 2; // use a smaller buffer size to check correct iteration.
        expect_error(
            "should be strictly increasing",
            [&]() -> void {
                test_validate(dir, opt);
            }
        );
    }

    // Check what happens if we insert an out-of-range value at the last position of each column/row.
    for (auto pos : std::vector<int>{2, 6, 10, 12, 13, 17}) {
        {
            initialize_directory_simple(dir, "compressed_sparse_matrix", "1.0");
            H5::H5File handle(dir / "matrix.h5", H5F_ACC_TRUNC);
            auto ghandle = handle.createGroup("compressed_sparse_matrix");
            auto icopy = indices;
            icopy[pos] = 15;
            mock_compressed_sparse_matrix(ghandle, dims, data, icopy, ptrs, options);
        }
        expect_validation_error(dir, "less than the number of");

        takane::Options opt;
        opt.hdf5_buffer_size = 2; // use a smaller buffer size to check correct iteration.
        expect_error(
            "less than the number of",
            [&]() -> void {
                test_validate(dir, opt);
            }
        );
    }
}

INSTANTIATE_TEST_SUITE_P(
    CompressedSparseMatrix,
    CompressedSparseMatrixIndicesErrorTest,
    ::testing::Values(true, false)
);

/**********************************/

TEST(CompressedSparseMatrix, NamesOkay) {
    auto dir = define_test_path("compressed_sparse_matrix");

    {
        auto ghandle = mock_compressed_sparse_matrix(dir, CompressedSparseMatrixOptions(55, 33));
        auto nhandle = ghandle.createGroup("names");
        nhandle.createDataSet("0", H5::StrType(0, 5), create_hdf5_dataspace(55));
        nhandle.createDataSet("1", H5::StrType(0, 5), create_hdf5_dataspace(33));
    }

    test_validate(dir);
    EXPECT_EQ(test_height(dir), 55);
    std::vector<std::size_t> expected_dims { 55, 33 };
    EXPECT_EQ(test_dimensions(dir), expected_dims);
}

TEST(CompressedSparseMatrix, NamesError) {
    auto dir = define_test_path("compressed_sparse_matrix");

    {
        auto ghandle = mock_compressed_sparse_matrix(dir, CompressedSparseMatrixOptions(55, 33));
        auto nhandle = ghandle.createGroup("names");
        nhandle.createDataSet("0", H5::StrType(0, 5), create_hdf5_dataspace(33));
        nhandle.createDataSet("1", H5::StrType(0, 5), create_hdf5_dataspace(55));
    }

    expect_validation_error(dir, "same length as the extent");
}
