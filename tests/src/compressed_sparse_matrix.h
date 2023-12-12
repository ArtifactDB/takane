#ifndef COMPRESSED_SPARSE_MATRIX_H
#define COMPRESSED_SPARSE_MATRIX_H

#include "H5Cpp.h"
#include <vector>
#include <random>
#include "utils.h"

namespace compressed_sparse_matrix {

struct Config {
    bool csc = true;
    bool as_integer = false;
};

static void mock(
    H5::Group& handle,
    std::vector<int> dimensions,
    std::vector<double> data,
    std::vector<int> indices,
    std::vector<int> indptr,
    Config config) 
{
    {
        hsize_t dim = 2;
        H5::DataSpace dspace(1, &dim);
        auto dhandle = handle.createDataSet("shape", H5::PredType::NATIVE_UINT32, dspace);
        dhandle.write(dimensions.data(), H5::PredType::NATIVE_INT);
    }

    if (config.csc) {
        hdf5_utils::attach_attribute(handle, "layout", "CSC");
    } else {
        hdf5_utils::attach_attribute(handle, "layout", "CSR");
    }

    if (config.as_integer) {
        auto dhandle = hdf5_utils::spawn_data(handle, "data", data.size(), H5::PredType::NATIVE_INT32);
        dhandle.write(data.data(), H5::PredType::NATIVE_DOUBLE);
        hdf5_utils::attach_attribute(handle, "type", "integer");
    } else {
        auto dhandle = hdf5_utils::spawn_data(handle, "data", data.size(), H5::PredType::NATIVE_DOUBLE);
        dhandle.write(data.data(), H5::PredType::NATIVE_DOUBLE);
        hdf5_utils::attach_attribute(handle, "type", "number");
    }

    {
        auto dhandle = hdf5_utils::spawn_data(handle, "indices", indices.size(), H5::PredType::NATIVE_UINT16);
        dhandle.write(indices.data(), H5::PredType::NATIVE_INT);
    }

    {
        auto dhandle = hdf5_utils::spawn_data(handle, "indptr", indptr.size(), H5::PredType::NATIVE_UINT32);
        dhandle.write(indptr.data(), H5::PredType::NATIVE_INT);
    }
}

void mock(const std::filesystem::path& path, int nr, int nc, double density, Config config = Config()) {
    initialize_directory_simple(path, "compressed_sparse_matrix", "1.0");

    int nprimary = (config.csc ? nc : nr);
    int nsecondary = (config.csc ? nr : nc);

    std::vector<double> data;
    std::vector<int> indices;
    std::vector<int> indptr(1);

    std::mt19937_64 rng(nr * nc + config.csc * 10 + config.as_integer);
    std::uniform_real_distribution<> sampling(0, 10);
    std::uniform_real_distribution<> udist;

    for (int p = 0; p < nprimary; ++p) {
        int count = 0;
        for (int s = 0; s < nsecondary; ++s) {
            if (udist(rng) < density) {
                data.push_back(sampling(rng));
                indices.push_back(s);
                ++count;
            }
        }
        indptr.push_back(indptr.back() + count);
    }

    H5::H5File handle(path / "matrix.h5", H5F_ACC_TRUNC);
    auto ghandle = handle.createGroup("compressed_sparse_matrix");
    mock(
        ghandle, 
        { nr, nc },
        data,
        indices,
        indptr,
        config
    );
}

}

#endif
