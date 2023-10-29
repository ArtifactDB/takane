#ifndef TAKANE_HDF5_DENSE_ARRAY_HPP
#define TAKANE_HDF5_DENSE_ARRAY_HPP

#include "H5Cpp.h"
#include "ritsuko/hdf5/hdf5.hpp"

#include "array.hpp"

/**
 * @file hdf5_dense_array.hpp
 * @brief Validation for HDF5 dense arrays.
 */

namespace takane {

namespace hdf5_dense_array {

/**
 * @brief Parameters for validating a HDF5 dense array file.
 */
struct Parameters {
    /**
     * Name of the dataset in the HDF5 file.
     */
    std::string dataset;

    /**
     * Expected dimensions of the dense array.
     * Dimensions are sorted from the fastest changing to the slowest changing.
     */
    std::vector<size_t> dimensions;

    /**
     * Expected type of the dense array.
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
     * Version of this file specification.
     */
    int version = 2;
};

/**
 * Validate a file containing a HDF5 dense array.
 *
 * @param handle Handle to the file.
 * @param params Validation parameters.
 */
inline void validate(const H5::H5File& handle, const Parameters& params) {
    const auto& dataset = params.dataset;
    if (!handle.exists(dataset) || handle.childObjType(dataset) != H5O_TYPE_DATASET) {
        throw std::runtime_error("expected a '" + dataset + "' dataset");
    }
    auto dhandle = handle.openDataSet(dataset);

    {
        const auto& dimensions = params.dimensions;
        auto dspace = dhandle.getSpace();

        size_t ndims = dspace.getSimpleExtentNdims();
        if (ndims != dimensions.size()) {
            throw std::runtime_error("unexpected number of dimensions for '" + dataset + "'");
        }

        std::vector<hsize_t> dims(ndims);
        dspace.getSimpleExtentDims(dims.data());

        for (size_t d = 0; d < ndims; ++d) {
            auto expected = dimensions[d];
            auto observed = dims[ndims - d - 1];
            if (observed != expected) {
                throw std::runtime_error("unexpected dimension extent for '" + dataset + "' (expected " + std::to_string(expected) + ", got " + std::to_string(observed) + ")");
            }
        }
    }

    {
        auto tclass = dhandle.getTypeClass();
        if (params.type == array::Type::INTEGER || params.type == array::Type::BOOLEAN) {
            if (tclass != H5T_INTEGER) {
                throw std::runtime_error("expected an integer type for '" + dataset + "'");
            }
        } else if (params.type == array::Type::NUMBER) {
            if (tclass != H5T_FLOAT && tclass != H5T_INTEGER) {
                throw std::runtime_error("expected an integer or floating-point type for '" + dataset + "'");
            }
        } else if (params.type == array::Type::STRING) {
            if (tclass != H5T_STRING) {
                throw std::runtime_error("expected a string type for '" + dataset + "'");
            }
        } else {
            throw std::runtime_error("not-yet-supported array type (" + std::to_string(static_cast<int>(params.type)) + ") for '" + dataset + "'");
        }
    }

    if (params.version >= 2) {
        const char* missing_attr = "missing-value-placeholder";
        if (dhandle.attrExists(missing_attr)) {
            ritsuko::hdf5::get_missing_placeholder_attribute(dhandle, missing_attr, dataset.c_str());
        }
    }

    if (params.has_dimnames) {
        array::check_dimnames(handle, params.dimnames_group, params.dimensions);
    }
}

/**
 * Validate a file containing a HDF5 dense array.
 *
 * @param path Path to the file.
 * @param params Validation parameters.
 */
inline void validate(const std::string& path, const Parameters& params) {
    H5::H5File handle(path, H5F_ACC_RDONLY);
    validate(handle, params);
}

}

}

#endif
