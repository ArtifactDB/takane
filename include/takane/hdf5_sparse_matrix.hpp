#ifndef TAKANE_HDF5_SPARSE_MATRIX_HPP
#define TAKANE_HDF5_SPARSE_MATRIX_HPP

#include "utils.hpp"
#include "ritsuko/hdf5/hdf5.hpp"

#include <stdexcept>

/**
 * @file hdf5_sparse_matrix.hpp
 * @brief Validation for HDF5 sparse matrices.
 */

namespace takane {

namespace hdf5_sparse_matrix {

/**
 * @brief Options for parsing the HDF5 sparse matrix file.
 */
struct Options {
    /**
     * Buffer size to use when reading values from the HDF5 file.
     */
    hsize_t buffer_size = 10000;

    /**
     * Version of the `hdf5_sparse_matrix` format.
     */
    int version = 2;
};

/**
 * Checks if a HDF5 sparse matrix is correctly formatted.
 * An error is raised if the file does not meet the specifications.
 *
 * @param handle Handle to the HDF5 file containing the matrix.
 * @param group Group containing the matrix data. 
 * @param dimensions Vector containing the matrix dimensions.
 * @param has_dimnames Whether the matrix contains dimnames.
 * @param dimnames_group Group containing the dimnames, if `has_dimnames = true`.
 * @param options Parsing options.
 */
inline void validate(
    const H5::H5File& handle, 
    const std::string& group, 
    const std::vector<size_t>& dimensions, 
    bool has_dimnames, 
    const std::string& dimnames_group, 
    Options options = Options()) 
{
    if (!handle.exists(group) || handle.childObjType(group) != H5O_TYPE_GROUP) {
        throw std::runtime_error("expected a '" + group + "' group");
    }
    auto dhandle = handle.openGroup(group);

    // Shape check.
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

        if (options.version >= 2) {
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

        auto block_size = ritsuko::hdf5::pick_1d_block_size(ixhandle.getCreatePlist(), num_nonzero, options.buffer_size);
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

    // Finally, checking the names.
    if (has_dimnames) {
        if (!handle.exists(dimnames_group) || handle.childObjType(dimnames_group) != H5O_TYPE_GROUP) {
            throw std::runtime_error("expected a '" + dimnames_group + "' group");
        }
        auto nhandle = handle.openGroup(dimnames_group);

        for (size_t i = 0; i < 2; ++i) {
            std::string dim_name = std::to_string(i);
            if (!nhandle.exists(dim_name)) {
                continue;
            }

            auto dset_name = dimnames_group + "/" + dim_name;
            if (nhandle.childObjType(dim_name) != H5O_TYPE_DATASET) {
                throw std::runtime_error("expected '" + dset_name + "' to be a dataset");
            }
            auto dnhandle = nhandle.openDataSet(dim_name);

            if (dnhandle.getTypeClass() != H5T_STRING) {
                throw std::runtime_error("expected '" + dset_name + "' to be a string dataset");
            }
            if (ritsuko::hdf5::get_1d_length(dnhandle.getSpace(), false, dset_name.c_str()) != dimensions[i]) {
                throw std::runtime_error("expected '" + dset_name + "' to have length equal to the corresponding dimension");
            }
        }
    }
}

/**
 * Overload for `hdf5_sparse_matrix::validate()` that accepts a file path.
 *
 * @tparam Args_ Arguments to be forwarded.
 *
 * @param path Path to the HDF5 file.
 * @param args Further arguments, passed to other `validate()` method.
 */
template<typename... Args_>
void validate(const std::string& path, Args_&& ... args) {
    H5::H5File handle(path, H5F_ACC_RDONLY);
    validate(handle, std::forward<Args_>(args)...);
}


}

}

#endif
