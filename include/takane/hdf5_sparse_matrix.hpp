#ifndef TAKANE_HDF5_SPARSE_MATRIX_HPP
#define TAKANE_HDF5_SPARSE_MATRIX_HPP

#include "utils.hpp"
#include "ritsuko/hdf5/hdf5.hpp"

#include "array.hpp"

#include <array>
#include <stdexcept>
#include <string>

/**
 * @file hdf5_sparse_matrix.hpp
 * @brief Validation for HDF5 sparse matrices.
 */

namespace takane {

namespace hdf5_sparse_matrix {

/**
 * @brief Parameters for validating a HDF5 sparse matrix file.
 */
struct Parameters {
    /**
     * @param group Name of the group containing the matrix.
     * @param dimensions Dimensions of the matrix.
     */
    Parameters(std::string group, std::array<size_t, 2> dimensions) : group(std::move(group)), dimensions(std::move(dimensions)) {}

    /**
     * Name of the group in the HDF5 file containing the matrix data.
     */
    std::string group;

    /**
     * Expected dimensions of the sparse matrix.
     * The first entry should contain the number of rows and the second should contain the number of columns.
     */
    std::array<size_t, 2> dimensions;

    /**
     * Expected type of the sparse matrix.
     */
    array::Type type = array::Type::INTEGER;

    /**
     * Whether the array has dimension names.
     */
    bool has_dimnames = false;

    /**
     * Name of the group containing the dimension names.
     */
    std::string dimnames_group;

    /**
     * Buffer size to use when reading values from the HDF5 file.
     */
    hsize_t buffer_size = 10000;

    /**
     * Version of this file specification.
     */
    int version = 2;
};

/**
 * Checks if a HDF5 sparse matrix is correctly formatted.
 * An error is raised if the file does not meet the specifications.
 *
 * @param handle Handle to the HDF5 file containing the matrix.
 * @param params Parameters for validation.
 */
inline void validate(const H5::H5File& handle, const Parameters& params) {
    const auto& group = params.group;    
    if (!handle.exists(group) || handle.childObjType(group) != H5O_TYPE_GROUP) {
        throw std::runtime_error("expected a '" + group + "' group");
    }
    auto dhandle = handle.openGroup(group);

    // Shape check.
    const auto& dimensions = params.dimensions;    
    {
        std::string dset_name = group + "/shape";
        if (!dhandle.exists("shape") || dhandle.childObjType("shape") != H5O_TYPE_DATASET) {
            throw std::runtime_error("expected a '" + dset_name + "' dataset");
        }

        auto shandle = dhandle.openDataSet("shape");
        if (shandle.getTypeClass() != H5T_INTEGER) {
            throw std::runtime_error("expected the '" + dset_name + "' dataset to be integer");
        }

        size_t len = ritsuko::hdf5::get_1d_length(shandle.getSpace(), false, dset_name.c_str());
        if (len != 2) {
            throw std::runtime_error("expected the '" + dset_name + "' dataset to be length 2");
        }

        hsize_t dims[2];
        shandle.read(dims, H5::PredType::NATIVE_HSIZE);
        if (dims[0] != dimensions[0]) {
            throw std::runtime_error("mismatch in first entry of '" + dset_name + "' (expected " + std::to_string(dimensions[0]) + ", got " + std::to_string(dims[0]) + ")");
        }
        if (dims[1] != dimensions[1]) {
            throw std::runtime_error("mismatch in second entry of '" + dset_name + "' (expected " + std::to_string(dimensions[1]) + ", got " + std::to_string(dims[1]) + ")");
        }
    }

    size_t num_nonzero = 0;
    {
        std::string dset_name = group + "/data";
        if (!dhandle.exists("data") || dhandle.childObjType("data") != H5O_TYPE_DATASET) {
            throw std::runtime_error("expected a '" + dset_name + "' dataset");
        }

        auto ddhandle = dhandle.openDataSet("data");
        num_nonzero = ritsuko::hdf5::get_1d_length(ddhandle.getSpace(), false, dset_name.c_str());

        auto type_class = ddhandle.getTypeClass();
        if (type_class != H5T_INTEGER && type_class != H5T_FLOAT) {
            throw std::runtime_error("expected the '" + dset_name + "' dataset to be integer or float");
        }

        if (params.version >= 2) {
            const char* missing_attr = "missing-value-placeholder";
            if (ddhandle.attrExists(missing_attr)) {
                ritsuko::hdf5::get_missing_placeholder_attribute(ddhandle, missing_attr, dset_name.c_str());
            }
        }
    }

    // Okey dokey, need to load in the pointers.
    size_t primary_dim = dimensions[1];
    std::vector<hsize_t> indptrs;
    {
        std::string dset_name = group + "/indptr";
        if (!dhandle.exists("indptr") || dhandle.childObjType("indptr") != H5O_TYPE_DATASET) {
            throw std::runtime_error("expected a '" + dset_name + "' dataset");
        }

        auto iphandle = dhandle.openDataSet("indptr");
        if (iphandle.getTypeClass() != H5T_INTEGER) {
            throw std::runtime_error("expected the '" + dset_name + "' dataset to be integer");
        }

        size_t len = ritsuko::hdf5::get_1d_length(iphandle.getSpace(), false, dset_name.c_str());
        if (len != primary_dim + 1) {
            throw std::runtime_error("'" + dset_name + "' should have length equal to the number of columns plus 1");
        }

        indptrs.resize(len);
        iphandle.read(indptrs.data(), H5::PredType::NATIVE_HSIZE);

        if (indptrs[0] != 0) {
            throw std::runtime_error("first entry of '" + dset_name + "' should be zero");
        }
        if (indptrs.back() != num_nonzero) {
            throw std::runtime_error("last entry of '" + dset_name + "' should equal the number of non-zero elements");
        }

        for (size_t i = 1; i < len; ++i) {
            if (indptrs[i] < indptrs[i-1]) {
                throw std::runtime_error("'" + dset_name + "' should be sorted in increasing order");
            }
        }
    }

    // Now iterating over the indices and checking that everything is kosher.
    {
        std::string dset_name = group + "/indices";
        if (!dhandle.exists("indices") || dhandle.childObjType("indices") != H5O_TYPE_DATASET) {
            throw std::runtime_error("expected a '" + dset_name + "' dataset");
        }

        auto ixhandle = dhandle.openDataSet("indices");
        if (num_nonzero != ritsuko::hdf5::get_1d_length(ixhandle.getSpace(), false, dset_name.c_str())) {
            throw std::runtime_error("'" + group + "/data' and '" + dset_name + "' should have the same length");
        }

        auto type_class = ixhandle.getTypeClass();
        if (type_class != H5T_INTEGER) { 
            throw std::runtime_error("expected the '" + dset_name + "' dataset to be integer or float");
        }

        auto block_size = ritsuko::hdf5::pick_1d_block_size(ixhandle.getCreatePlist(), num_nonzero, params.buffer_size);
        std::vector<hsize_t> buffer(block_size);
        size_t which_ptr = 0;
        hsize_t last_index = 0;
        auto limit = indptrs[0];

        ritsuko::hdf5::iterate_1d_blocks(
            num_nonzero,
            block_size,
            [&](hsize_t position, hsize_t len, const H5::DataSpace& memspace, const H5::DataSpace& dataspace) {
                buffer.resize(len);
                ixhandle.read(buffer.data(), H5::PredType::NATIVE_HSIZE, memspace, dataspace);

                for (auto bIt = buffer.begin(); bIt != buffer.end(); ++bIt, ++position) {
                    if (*bIt >= dimensions[0]) {
                        throw std::runtime_error("out-of-range index in '" + dset_name + "'");
                    }

                    if (position == limit) {
                        // No need to check if there are more or fewer elements
                        // than expected, as we already know that indptr.back()
                        // is equal to the number of non-zero elements.
                        do {
                            ++which_ptr;
                            limit = indptrs[which_ptr];
                        } while (position == limit);

                    } else if (last_index >= *bIt) {
                        throw std::runtime_error("indices in '" + dset_name + "' should be strictly increasing");
                    }

                    last_index = *bIt;
                }
            }
        );
    }

    if (params.has_dimnames) {
        array::check_dimnames(handle, params.dimnames_group, params.dimensions);
    }
}

/**
 * Overload for `hdf5_sparse_matrix::validate()` that accepts a file path.
 *
 * @param path Path to the HDF5 file.
 * @param params Parameters for validation.
 */
inline void validate(const char* path, const Parameters& params) {
    H5::H5File handle(path, H5F_ACC_RDONLY);
    validate(handle, params);
}


}

}

#endif
