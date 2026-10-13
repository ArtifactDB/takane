#ifndef MOCK_COMPRESSED_SPARSE_MATRIX_H
#define MOCK_COMPRESSED_SPARSE_MATRIX_H

#include "H5Cpp.h"

#include <vector>
#include <random>

#include "utils.h"

enum class CompressedSparseMatrixType {
    INTEGER,
    NUMBER,
    BOOLEAN
};

struct CompressedSparseMatrixBasicOptions {
    CompressedSparseMatrixBasicOptions() = default;
    CompressedSparseMatrixBasicOptions(bool column_sparse, CompressedSparseMatrixType type) : column_sparse(column_sparse), type(type) {}
public:
    bool column_sparse = true;
    CompressedSparseMatrixType type = CompressedSparseMatrixType::INTEGER;
};

template<typename Dimension_, typename Data_, typename Index_, typename Pointer_>
void mock_compressed_sparse_matrix(
    H5::Group& handle,
    const std::vector<Dimension_>& dimensions,
    const std::vector<Data_>& data,
    const std::vector<Index_>& indices,
    const std::vector<Pointer_>& indptr,
    const CompressedSparseMatrixBasicOptions& options
) {
    add_hdf5_numeric_dataset(handle, "shape", H5::PredType::NATIVE_UINT32, dimensions);

    if (options.column_sparse) {
        add_hdf5_string_attribute(handle, "layout", "CSC");
    } else {
        add_hdf5_string_attribute(handle, "layout", "CSR");
    }

    if (options.type == CompressedSparseMatrixType::NUMBER) {
        add_hdf5_numeric_dataset(handle, "data", H5::PredType::NATIVE_DOUBLE, data);
        add_hdf5_string_attribute(handle, "type", "number");
    } else if (options.type == CompressedSparseMatrixType::INTEGER) {
        add_hdf5_numeric_dataset(handle, "data", H5::PredType::NATIVE_INT32, data);
        add_hdf5_string_attribute(handle, "type", "integer");
    } else {
        add_hdf5_numeric_dataset(handle, "data", H5::PredType::NATIVE_INT8, data);
        add_hdf5_string_attribute(handle, "type", "boolean");
    }

    add_hdf5_numeric_dataset(handle, "indices", H5::PredType::NATIVE_UINT32, indices);
    add_hdf5_numeric_dataset(handle, "indptr", H5::PredType::NATIVE_UINT64, indptr);
}

struct CompressedSparseMatrixOptions : public CompressedSparseMatrixBasicOptions {
    CompressedSparseMatrixOptions(int num_rows, int num_columns) : num_rows(num_rows), num_columns(num_columns) {}
public:
    int num_rows;
    int num_columns;
    double density = 0.2;
};

inline H5::Group mock_compressed_sparse_matrix(const std::filesystem::path& path, const CompressedSparseMatrixOptions& options) {
    initialize_directory_simple(path, "compressed_sparse_matrix", "1.0");

    const int nr = options.num_rows;
    const int nc = options.num_columns;
    const int nprimary = (options.column_sparse ? nc : nr);
    const int nsecondary = (options.column_sparse ? nr : nc);

    std::vector<double> data;
    std::vector<int> indices;
    std::vector<int> indptr(1);

    std::mt19937_64 rng(nr * nc + options.density * 10 + static_cast<int>(options.type) * 7 + static_cast<int>(options.column_sparse) * 3);
    std::uniform_real_distribution<> sampling(0, 10);
    std::uniform_real_distribution<> udist;

    for (int p = 0; p < nprimary; ++p) {
        int count = 0;
        for (int s = 0; s < nsecondary; ++s) {
            if (udist(rng) < options.density) {
                data.push_back(sampling(rng));
                indices.push_back(s);
                ++count;
            }
        }
        indptr.push_back(indptr.back() + count);
    }

    H5::H5File handle(path / "matrix.h5", H5F_ACC_TRUNC);
    auto ghandle = handle.createGroup("compressed_sparse_matrix");
    mock_compressed_sparse_matrix(
        ghandle, 
        std::vector<int>{ nr, nc },
        data,
        indices,
        indptr,
        options
    );

    return ghandle;
}

#endif
