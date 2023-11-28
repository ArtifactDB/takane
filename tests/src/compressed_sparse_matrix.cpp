#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "takane/takane.hpp"
#include "compressed_sparse_matrix.h"
#include "utils.h"

#include <numeric>
#include <string>
#include <vector>
#include <random>

struct SparseMatrixTest : public ::testing::Test {
    SparseMatrixTest() {
        path = "TEST_sparse_matrix";
        name = "compressed_sparse_matrix";
    }

    std::filesystem::path path;
    std::string name;

    H5::H5File reopen() {
        return H5::H5File(path / "matrix.h5", H5F_ACC_RDWR);
    }

public:
    void expect_error(const std::string& msg) {
        EXPECT_ANY_THROW({
            try {
                takane::validate(path);
            } catch (std::exception& e) {
                EXPECT_THAT(e.what(), ::testing::HasSubstr(msg));
                throw;
            }
        });
    }
};

TEST_F(SparseMatrixTest, Basic) {
    {
        initialize_directory(path, name);
        H5::H5File handle(path / "matrix.h5", H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup(name);
        hdf5_utils::attach_attribute(ghandle, "version", "2.0");
    }
    expect_error("unsupported version");

    // Success with lots of zero-length columns.
    compressed_sparse_matrix::mock(path, 20, 30, 0.02);
    takane::validate(path);
    EXPECT_EQ(takane::height(path), 20);

    std::vector<size_t> expected_dims { 20, 30 };
    EXPECT_EQ(takane::dimensions(path), expected_dims);

    // Success with no zero-length columns.
    compressed_sparse_matrix::mock(path, 20, 30, 0.5);
    takane::validate(path);
}

TEST_F(SparseMatrixTest, Layout) {
    // Row-major layout:
    compressed_sparse_matrix::Config config;
    config.csc = false;
    compressed_sparse_matrix::mock(path, 40, 50, 0.2, config);
    takane::validate(path);
    EXPECT_EQ(takane::height(path), 40);

    // Fails with unknown layout: 
    {
        auto handle = reopen();
        auto ghandle = handle.openGroup("compressed_sparse_matrix");
        ghandle.removeAttr("layout");
        hdf5_utils::attach_attribute(ghandle, "layout", "fooobar");
    }
    expect_error("'layout' attribute must be");
}

TEST_F(SparseMatrixTest, Shape) {
    {
        compressed_sparse_matrix::mock(path, 20, 30, 0.2);
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        ghandle.unlink("shape");
        hdf5_utils::spawn_data(ghandle, "shape", 2, H5::PredType::NATIVE_INT32);
    }
    expect_error("64-bit unsigned integer");

    {
        compressed_sparse_matrix::mock(path, 20, 30, 0.2);
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        ghandle.unlink("shape");
        hdf5_utils::spawn_data(ghandle, "shape", 3, H5::PredType::NATIVE_UINT32);
    }
    expect_error("length 2");
}

TEST_F(SparseMatrixTest, Data) {
    // Trying with integers.
    {
        compressed_sparse_matrix::Config config;
        config.as_integer = true;
        compressed_sparse_matrix::mock(path, 20, 30, 0.2, config);
    }
    takane::validate(path);

    // Still good for booleans.
    {
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        ghandle.removeAttr("type");
        hdf5_utils::attach_attribute(ghandle, "type", "boolean");
    }
    takane::validate(path);

    // Now checking the various failures.
    {
        compressed_sparse_matrix::mock(path, 20, 30, 0.2);
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        ghandle.removeAttr("type");
        hdf5_utils::attach_attribute(ghandle, "type", "integer");
    }
    expect_error("32-bit signed integer");

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        ghandle.removeAttr("type");
        hdf5_utils::attach_attribute(ghandle, "type", "boolean");
    }
    expect_error("32-bit signed integer");

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        size_t len = ritsuko::hdf5::get_1d_length(ghandle.openDataSet("data"), false);
        ghandle.unlink("data");
        hdf5_utils::spawn_data(ghandle, "data", len, H5::PredType::NATIVE_INT64);
        ghandle.removeAttr("type");
        hdf5_utils::attach_attribute(ghandle, "type", "number");
    }
    expect_error("64-bit float");

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        ghandle.removeAttr("type");
        hdf5_utils::attach_attribute(ghandle, "type", "YAYYA");
    }
    expect_error("unknown matrix type");
}

TEST_F(SparseMatrixTest, MissingPlaceholder) {
    {
        compressed_sparse_matrix::Config config;
        config.csc = false;
        config.as_integer = true;
        compressed_sparse_matrix::mock(path, 99, 20, 0.2, config);
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        auto dhandle = ghandle.openDataSet("data");
        dhandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_FLOAT, H5S_SCALAR);
    }
    expect_error("same type as");

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        auto dhandle = ghandle.openDataSet("data");
        dhandle.removeAttr("missing-value-placeholder"); 
        dhandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_INT32, H5S_SCALAR);
    }
    takane::validate(path);
}

TEST_F(SparseMatrixTest, IndptrFails) {
    std::vector<int> expected;

    {
        compressed_sparse_matrix::mock(path, 50, 30, 0.25);
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        {
            auto dhandle = ghandle.openDataSet("indptr");
            expected.resize(31);
            dhandle.read(expected.data(), H5::PredType::NATIVE_INT);
        }
        ghandle.unlink("indptr");
        hdf5_utils::spawn_data(ghandle, "indptr", 31, H5::PredType::NATIVE_INT32);
    }
    expect_error("64-bit unsigned integer");

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        ghandle.unlink("indptr");
        hdf5_utils::spawn_data(ghandle, "indptr", 30, H5::PredType::NATIVE_UINT32);
    }
    expect_error("should have length");

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        ghandle.unlink("indptr");
        auto dhandle = hdf5_utils::spawn_data(ghandle, "indptr", 31, H5::PredType::NATIVE_UINT32);
        auto copy = expected;
        copy[0] = 1;
        dhandle.write(copy.data(), H5::PredType::NATIVE_INT);
    }
    expect_error("first entry should be zero");

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        auto dhandle = ghandle.openDataSet("indptr");
        auto copy = expected;
        copy.back() -= 1;
        dhandle.write(copy.data(), H5::PredType::NATIVE_INT);
    }
    expect_error("last entry");

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        auto dhandle = ghandle.openDataSet("indptr");
        auto copy = expected;
        std::reverse(copy.begin(), copy.end());
        std::swap(copy.front(), copy.back());
        dhandle.write(copy.data(), H5::PredType::NATIVE_INT);
    }
    expect_error("should be sorted");
}
TEST_F(SparseMatrixTest, IndicesFails) {
    std::vector<int> expected;

    {
        compressed_sparse_matrix::mock(path, 27, 43, 0.25);
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        {
            auto dhandle = ghandle.openDataSet("indices");
            expected.resize(ritsuko::hdf5::get_1d_length(dhandle, false));
            dhandle.read(expected.data(), H5::PredType::NATIVE_INT);
        }
        ghandle.unlink("indices");
        hdf5_utils::spawn_data(ghandle, "indices", expected.size(), H5::PredType::NATIVE_INT32);
    }
    expect_error("64-bit unsigned integer");

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        ghandle.unlink("indices");
        hdf5_utils::spawn_data(ghandle, "indices", expected.size() + 1, H5::PredType::NATIVE_UINT32);
    }
    expect_error("number of non-zero elements");

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        ghandle.unlink("indices");
        auto dhandle = hdf5_utils::spawn_data(ghandle, "indices", expected.size(), H5::PredType::NATIVE_UINT32);
        auto copy = expected;
        copy[copy.size() / 2] = 27;
        dhandle.write(copy.data(), H5::PredType::NATIVE_INT);
    }
    expect_error("out-of-range");

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        ghandle.unlink("indices");
        auto dhandle = hdf5_utils::spawn_data(ghandle, "indices", expected.size(), H5::PredType::NATIVE_UINT32);
        auto copy = expected;
        std::reverse(copy.begin(), copy.end());
        dhandle.write(copy.data(), H5::PredType::NATIVE_INT);
    }
    expect_error("strictly increasing");
}

TEST_F(SparseMatrixTest, Names) {
    {
        compressed_sparse_matrix::mock(path, 55, 33, 0.25);
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        auto nhandle = ghandle.createGroup("names");
        hdf5_utils::spawn_data(nhandle, "0", 20, H5::StrType(0, 5));
    }
    expect_error("same length as the extent");

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        auto nhandle = ghandle.openGroup("names");
        nhandle.unlink("0");
        hdf5_utils::spawn_data(nhandle, "1", 33, H5::StrType(0, 5));
    }
    takane::validate(path);
}
