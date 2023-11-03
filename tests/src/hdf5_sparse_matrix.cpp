#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "takane/hdf5_sparse_matrix.hpp"

#include <numeric>
#include <string>
#include <vector>
#include <random>

struct Hdf5SparseMatrixTest : public ::testing::TestWithParam<int> {
    Hdf5SparseMatrixTest() {
        path = "TEST-hdf5_sparse_matrix.h5";
        name = "matrix";
    }

    std::string path, name;

public:
    static void create_hdf5_sparse_matrix(
        H5::Group& handle,
        std::vector<int> dimensions,
        std::vector<double> data,
        std::vector<int> indices,
        std::vector<int> indptr,
        int version,
        bool as_int = true
    ) {
        {
            hsize_t dim = 2;
            H5::DataSpace dspace(1, &dim);
            auto dhandle = handle.createDataSet("shape", H5::PredType::NATIVE_INT, dspace);
            dhandle.write(dimensions.data(), H5::PredType::NATIVE_INT);
        }

        if (version >= 3) {
            H5::StrType stype(0, H5T_VARIABLE);
            auto ahandle = handle.createAttribute("version", stype, H5S_SCALAR);
            ahandle.write(stype, std::string("1.0"));

            auto fhandle = handle.createAttribute("format", stype, H5S_SCALAR);
            fhandle.write(stype, std::string("tenx_matrix"));
        }

        {
            hsize_t dim = data.size();
            H5::DataSpace dspace(1, &dim);
            if (as_int) {
                auto dhandle = handle.createDataSet("data", H5::PredType::NATIVE_INT, dspace);
                dhandle.write(data.data(), H5::PredType::NATIVE_DOUBLE);
            } else {
                auto dhandle = handle.createDataSet("data", H5::PredType::NATIVE_DOUBLE, dspace);
                dhandle.write(data.data(), H5::PredType::NATIVE_DOUBLE);
            }
        }

        {
            hsize_t dim = indices.size();
            H5::DataSpace dspace(1, &dim);
            auto dhandle = handle.createDataSet("indices", H5::PredType::NATIVE_UINT16, dspace);
            dhandle.write(indices.data(), H5::PredType::NATIVE_INT);
        }

        {
            hsize_t dim = indptr.size();
            H5::DataSpace dspace(1, &dim);
            auto dhandle = handle.createDataSet("indptr", H5::PredType::NATIVE_UINT32, dspace);
            dhandle.write(indptr.data(), H5::PredType::NATIVE_INT);
        }
    }

    static void create_hdf5_sparse_matrix(H5::Group& handle, int version) {
        create_hdf5_sparse_matrix(
            handle, 
            { 12, 18 }, 
            { 4,4,10,6,0,3,16,9,2,6,6,1,6,1,2,1,2,1,7,4,1,1,5,5,2,3,0,3,5,2,2,6,0,2,8,6,7,4,5,0,2,4,1 },
            { 0,3,9,11,2,3,5,8,2,9,11,2,4,9,1,5,9,0,2,4,5,10,0,3,10,1,3,8,5,7,9,7,9,1,3,1,6,9,11,10,11,2,5 },
            { 0,4,8,11,11,11,14,14,17,22,25,28,31,33,35,39,41,41,43 },
            version
        );
    }

    template<typename ... Args_>
    static void expect_error(const std::string& msg, const std::string& path, Args_&& ... args) {
        EXPECT_ANY_THROW({
            try {
                takane::hdf5_sparse_matrix::validate(path.c_str(), std::forward<Args_>(args)...);
            } catch (std::exception& e) {
                EXPECT_THAT(e.what(), ::testing::HasSubstr(msg));
                throw;
            }
        });
    }
};

TEST_P(Hdf5SparseMatrixTest, Success) {
    auto version = GetParam();

    // Lots of zero-length columns.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup(name);
        create_hdf5_sparse_matrix(
            ghandle, 
            { 10, 20 }, 
            { 0.055, -0.8, -0.71, 1.1, -0.86, -0.47, 1.5, -0.034, -0.7, 0.022, 0.49, 0.39, -0.87, 0.33, -0.5, -0.47, -0.0058, 0.15, 0.71, -0.24 },
            { 0, 5, 5, 7, 8, 3, 9, 8, 9, 6, 7, 1, 5, 7, 6, 0, 6, 3, 6, 5 },
            { 0, 0, 2, 3, 3, 5, 7, 7, 9, 10, 10, 10, 11, 14, 15, 17, 17, 17, 17, 19, 20 },
            version,
            false
        );
    }
    {
        takane::hdf5_sparse_matrix::Parameters params(name, { 10, 20 });
        params.type = takane::array::Type::NUMBER;
        params.version = version;
        takane::hdf5_sparse_matrix::validate(path.c_str(), params);
    }

    // No zero-length columns.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup(name);
        create_hdf5_sparse_matrix(
            ghandle, 
            { 20, 10 }, 
            { -2,-1,-9,15,9,15,-4,0,-9,-8,-4,20,-3,6,-11,3,11,-17,-9,4,-10,9,1,-16,-8,-9,2,15,9,-20,12,5,-2,-8,-2,-13,6,14,15,-7 },
            { 3,12,19,0,2,7,14,0,2,3,11,14,17,10,0,1,10,13,14,3,5,7,8,9,12,15,3,4,7,9,10,14,15,11,16,0,5,6,8,12 },
            { 0,3,7,13,14,19,22,26,33,35,40 },
            version
        );
    }
    {
        takane::hdf5_sparse_matrix::Parameters params(name, { 20, 10 });
        params.version = version;
        takane::hdf5_sparse_matrix::validate(path.c_str(), params);

        params.type = takane::array::Type::NUMBER; // still works for floats.
        takane::hdf5_sparse_matrix::validate(path.c_str(), params);

        params.type = takane::array::Type::BOOLEAN; // still works for bools, I guess.
        takane::hdf5_sparse_matrix::validate(path.c_str(), params);
    }

    // Has a missing value.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup(name);
        create_hdf5_sparse_matrix(
            ghandle, 
            { 20, 10 }, 
            { -2,-1,-9,15,9,15,-4,0,-9,-8,-4,20,-3,6,-11,3,11,-17,-9,4,-10,9,1,-16,-8,-9,2,15,9,-20,12,5,-2,-8,-2,-13,6,14,15,-7 },
            { 3,12,19,0,2,7,14,0,2,3,11,14,17,10,0,1,10,13,14,3,5,7,8,9,12,15,3,4,7,9,10,14,15,11,16,0,5,6,8,12 },
            { 0,3,7,13,14,19,22,26,33,35,40 },
            version
        );
        auto dhandle = ghandle.openDataSet("data");
        dhandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_INT, H5S_SCALAR);
    }
    {
        takane::hdf5_sparse_matrix::Parameters params(name, { 20, 10 });
        params.version = version;
        takane::hdf5_sparse_matrix::validate(path.c_str(), params);

        params.type = takane::array::Type::NUMBER;
        takane::hdf5_sparse_matrix::validate(path.c_str(), params);
    }
}

TEST_P(Hdf5SparseMatrixTest, BasicFails) {
    auto version = GetParam();
    takane::hdf5_sparse_matrix::Parameters params(name, {12, 18});
    params.version = version;

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
    }
    expect_error("expected a 'matrix' group", path, params);

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        handle.createDataSet(name, H5::PredType::NATIVE_INT, H5S_SCALAR);
    }
    expect_error("expected a 'matrix' group", path, params);
}

TEST_P(Hdf5SparseMatrixTest, ShapeFails) {
    auto version = GetParam();
    takane::hdf5_sparse_matrix::Parameters params(name, {12, 18});
    params.version = version;

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup(name);
        create_hdf5_sparse_matrix(ghandle, version);
        ghandle.unlink("shape");
    }
    expect_error("expected a dataset", path, params);

    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        auto ghandle = handle.openGroup(name);
        ghandle.createGroup("shape");
    }
    expect_error("expected a dataset", path, params);

    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        auto ghandle = handle.openGroup(name);
        ghandle.unlink("shape");
        ghandle.createDataSet("shape", H5::PredType::NATIVE_FLOAT, H5S_SCALAR);
    }
    if (version < 3) {
        expect_error("expected an integer dataset", path, params);
    } else {
        expect_error("64-bit signed integer", path, params);
    }

    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        auto ghandle = handle.openGroup(name);
        ghandle.unlink("shape");
        hsize_t dims = 3;
        H5::DataSpace dspace(1, &dims);
        ghandle.createDataSet("shape", H5::PredType::NATIVE_INT, dspace);
    }
    expect_error("length 2", path, params);

    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        auto ghandle = handle.openGroup(name);
        ghandle.unlink("shape");
        hsize_t dims = 2;
        H5::DataSpace dspace(1, &dims);
        auto shandle = ghandle.createDataSet("shape", H5::PredType::NATIVE_INT, dspace);
        std::vector<int> shape { 10, 20 };
        shandle.write(shape.data(), H5::PredType::NATIVE_INT);
    }
    expect_error("first entry", path, params);
    params.dimensions[0] = 10;
    expect_error("second entry", path, params);
}

TEST_P(Hdf5SparseMatrixTest, DataFails) {
    auto version = GetParam();
    takane::hdf5_sparse_matrix::Parameters params(name, {12, 18});
    params.version = version;

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup(name);
        create_hdf5_sparse_matrix(ghandle, version);
    }
    {
        params.type = takane::array::Type::STRING;
        expect_error("unexpected array type", path, params);
        params.type = takane::array::Type::INTEGER;
    }

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup(name);
        create_hdf5_sparse_matrix(ghandle, version);
        ghandle.unlink("data");
    }
    expect_error("expected a dataset", path, params);

    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        auto ghandle = handle.openGroup(name);
        ghandle.createGroup("data");
    }
    expect_error("expected a dataset", path, params);

    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        auto ghandle = handle.openGroup(name);
        ghandle.unlink("data");
        hsize_t dim = 10;
        auto dspace = H5::DataSpace(1, &dim);
        ghandle.createDataSet("data", H5::StrType(0, H5T_VARIABLE), dspace);
    }
    if (version < 3) {
        expect_error("expected an integer dataset", path, params);
    } else {
        expect_error("subset of a 32-bit signed integer", path, params);
        params.type = takane::array::Type::NUMBER;
        expect_error("subset of a 64-bit float", path, params);
        params.type = takane::array::Type::INTEGER;
    }

    if (version >= 2) {
        {
            H5::H5File handle(path, H5F_ACC_RDWR);
            auto ghandle = handle.openGroup(name);
            ghandle.unlink("data");
            hsize_t dim = ritsuko::hdf5::get_1d_length(ghandle.openDataSet("indices").getSpace(), false);
            auto dspace = H5::DataSpace(1, &dim);
            auto dhandle = ghandle.createDataSet("data", H5::PredType::NATIVE_INT, dspace);
            dhandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_DOUBLE, H5S_SCALAR);
        }
        expect_error("missing-value-placeholder", path, params);
    }
}

TEST_P(Hdf5SparseMatrixTest, IndptrFails) {
    auto version = GetParam();
    takane::hdf5_sparse_matrix::Parameters params(name, {12, 18});
    params.version = version;

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup(name);
        create_hdf5_sparse_matrix(ghandle, version);
        ghandle.unlink("indptr");
    }
    expect_error("expected a dataset", path, params);

    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        auto ghandle = handle.openGroup(name);
        ghandle.createGroup("indptr");
    }
    expect_error("expected a dataset", path, params);

    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        auto ghandle = handle.openGroup(name);
        ghandle.unlink("indptr");
        hsize_t dim = 10;
        auto dspace = H5::DataSpace(1, &dim);
        ghandle.createDataSet("indptr", H5::StrType(0, H5T_VARIABLE), dspace);
    }
    if (version < 3) {
        expect_error("expected an integer dataset", path, params);
    } else {
        expect_error("subset of a 64-bit unsigned integer", path, params);
    }

    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        auto ghandle = handle.openGroup(name);
        ghandle.unlink("indptr");
        hsize_t dim = 17;
        auto dspace = H5::DataSpace(1, &dim);
        ghandle.createDataSet("indptr", H5::PredType::NATIVE_UINT16, dspace);
    }
    expect_error("should have length", path, params);

    hsize_t dim = 19;
    std::vector<int> new_indptrs(dim);
    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        auto ghandle = handle.openGroup(name);
        ghandle.unlink("indptr");

        auto dspace = H5::DataSpace(1, &dim);
        auto dhandle = ghandle.createDataSet("indptr", H5::PredType::NATIVE_UINT16, dspace);
        std::fill(new_indptrs.begin(), new_indptrs.end(), 1);
        dhandle.write(new_indptrs.data(), H5::PredType::NATIVE_INT);
    }
    expect_error("first entry", path, params);

    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        auto ghandle = handle.openGroup(name);
        auto dhandle = ghandle.openDataSet("indptr");
        std::fill(new_indptrs.begin(), new_indptrs.end(), 0);
        dhandle.write(new_indptrs.data(), H5::PredType::NATIVE_INT);
    }
    expect_error("last entry", path, params);

    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        auto ghandle = handle.openGroup(name);
        auto dhandle = ghandle.openDataSet("indptr");

        // Injecting some shenanigans in here.
        new_indptrs[5] = 1;
        new_indptrs.back() = 43;
        dhandle.write(new_indptrs.data(), H5::PredType::NATIVE_INT);
    }
    expect_error("sorted in increasing order", path, params);
}

TEST_P(Hdf5SparseMatrixTest, IndicesFails) {
    auto version = GetParam();
    takane::hdf5_sparse_matrix::Parameters params(name, {12, 18});
    params.version = version;

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup(name);
        create_hdf5_sparse_matrix(ghandle, version);
        ghandle.unlink("indices");
    }
    expect_error("expected a dataset", path, params);

    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        auto ghandle = handle.openGroup(name);
        ghandle.createGroup("indices");
    }
    expect_error("expected a dataset", path, params);

    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        auto ghandle = handle.openGroup(name);
        ghandle.unlink("indices");

        hsize_t dim = 40;
        auto dspace = H5::DataSpace(1, &dim);
        ghandle.createDataSet("indices", H5::PredType::NATIVE_INT, dspace);
    }
    expect_error("should be equal to the number of non-zero elements", path, params);

    hsize_t dim = 43;
    std::vector<int> indices(dim);
    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        auto ghandle = handle.openGroup(name);
        ghandle.unlink("indices");

        auto dspace = H5::DataSpace(1, &dim);
        auto dhandle = ghandle.createDataSet("indices", H5::PredType::NATIVE_UINT16, dspace);
        dhandle.write(indices.data(), H5::PredType::NATIVE_INT);
    }
    expect_error("should be strictly increasing", path, params);

    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        auto ghandle = handle.openGroup(name);
        ghandle.unlink("indices");

        auto dspace = H5::DataSpace(1, &dim);
        auto dhandle = ghandle.createDataSet("indices", H5::PredType::NATIVE_UINT16, dspace);
        indices[1] = 12;
        dhandle.write(indices.data(), H5::PredType::NATIVE_INT);
    }
    expect_error("out-of-range", path, params);
}
    
TEST_P(Hdf5SparseMatrixTest, IndicesBlocked) {
    auto version = GetParam();

    std::vector<int> indices;
    std::vector<int> indptr(1);
    std::vector<double> data;

    size_t nc = 100, nr = 1000;
    std::mt19937_64 rng(42);
    std::uniform_real_distribution dist;
    for (size_t c = 0; c < nc; ++c) {
        for (size_t r = 0; r < nr; ++r) {
            if (dist(rng) < 0.2) {
                indices.push_back(r);
            }
        }
        indptr.push_back(indices.size());
    }
    data.resize(indices.size());

    std::string path = "TEST-hdf5_sparse_matrix.h5";
    std::string name = "matrix";
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup(name);
        create_hdf5_sparse_matrix(
            ghandle,
            std::vector<int>{ int(nr), int(nc) },
            data,
            indices,
            indptr,
            version
        );
    }

    // Now checking that we do the right thing at each buffer size.
    takane::hdf5_sparse_matrix::Parameters params(name, { nr, nc });
    params.version = version;

    params.buffer_size = 10;
    takane::hdf5_sparse_matrix::validate(path.c_str(), params);

    params.buffer_size = 50;
    takane::hdf5_sparse_matrix::validate(path.c_str(), params);

    params.buffer_size = 100;
    takane::hdf5_sparse_matrix::validate(path.c_str(), params);

    params.buffer_size = 1000;
    takane::hdf5_sparse_matrix::validate(path.c_str(), params);

    params.buffer_size = 10000;
    takane::hdf5_sparse_matrix::validate(path.c_str(), params);
}

TEST_P(Hdf5SparseMatrixTest, NameCheck) {
    auto version = GetParam();
    takane::hdf5_sparse_matrix::Parameters params(name, {12, 18});
    params.version = version;
    params.has_dimnames = true;

    std::string dname;
    if (version < 3) {
        dname = "dimnames";
        params.dimnames_group = dname;
    } else {
        dname = name + "/names";
    }

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup(name);
        create_hdf5_sparse_matrix(ghandle, version);
        handle.createGroup(dname);
    }
    takane::hdf5_sparse_matrix::validate(path.c_str(), params);

    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        auto nhandle = handle.openGroup(dname);
        hsize_t dim = 18;
        H5::DataSpace dspace(1, &dim);
        nhandle.createDataSet("1", H5::StrType(0, H5T_VARIABLE), dspace);
    }
    takane::hdf5_sparse_matrix::validate(path.c_str(), params);

    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        auto nhandle = handle.openGroup(dname);
        hsize_t dim = 12;
        H5::DataSpace dspace(1, &dim);
        nhandle.createDataSet("0", H5::StrType(0, H5T_VARIABLE), dspace);
    }
    takane::hdf5_sparse_matrix::validate(path.c_str(), params);

    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        handle.unlink(dname);
    }
    expect_error("expected a group", path, params);

    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        auto ghandle = handle.createGroup(dname);
        ghandle.createGroup("0");
    }
    expect_error("expected '0' to be a dataset", path, params);

    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        auto ghandle = handle.openGroup(dname);
        ghandle.unlink("0");
        ghandle.createDataSet("0", H5::PredType::NATIVE_INT, H5S_SCALAR);
    }
    expect_error("expected '0' to be a string dataset", path, params);

    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        auto ghandle = handle.openGroup(dname);
        ghandle.unlink("0");
        hsize_t dim = 10;
        H5::DataSpace dspace(1, &dim);
        ghandle.createDataSet("0", H5::StrType(0, H5T_VARIABLE), dspace);
    }
    expect_error("expected '0' to have length", path, params);

    if (version >= 3) {
        auto params2 = params;
        params2.has_dimnames = false;
        expect_error("unexpected 'names'", path, params2);
    }
}

TEST_P(Hdf5SparseMatrixTest, General) {
    auto version = GetParam();
    takane::hdf5_sparse_matrix::Parameters params(name, {12, 18});
    params.version = version;

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
    }
    expect_error("'" + name + "' group", path.c_str(), params);

    H5::StrType stype(0, H5T_VARIABLE);
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup(name);
        auto attr = ghandle.createAttribute("version", stype, H5S_SCALAR);
        attr.write(stype, std::string("2.0"));
    }
    expect_error("unsupported version", path.c_str(), params);

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup(name);
        auto attr = ghandle.createAttribute("version", stype, H5S_SCALAR);
        attr.write(stype, std::string("1.0"));
        auto fattr = ghandle.createAttribute("format", stype, H5S_SCALAR);
        fattr.write(stype, std::string("whee"));
    }
    expect_error("unsupported format", path.c_str(), params);
}

INSTANTIATE_TEST_SUITE_P(
    Hdf5SparseMatrix,
    Hdf5SparseMatrixTest,
    ::testing::Values(1,2,3) // versions
);
