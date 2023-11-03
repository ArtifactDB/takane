#ifndef TAKANE_ARRAY_HPP
#define TAKANE_ARRAY_HPP

#include "H5Cpp.h"
#include "ritsuko/hdf5/hdf5.hpp"

namespace takane {

/**
 * @namespace takane::array
 * @brief Definitions for abstract arrays.
 */
namespace array {

/**
 * Type of the data values in the array.
 */
enum Type {
    INTEGER,
    NUMBER,
    BOOLEAN,
    STRING
};

/**
 * @cond
 */
inline void check_dimnames(const H5::DataSet& handle, size_t expected_dim) try {
    if (handle.getTypeClass() != H5T_STRING) {
        throw std::runtime_error("expected a string dataset");
    }
    if (ritsuko::hdf5::get_1d_length(handle.getSpace(), false) != expected_dim) {
        throw std::runtime_error("expected dataset to have length equal to the corresponding dimension extent (" + std::to_string(expected_dim) + ")");
    }
} catch (std::exception& e) {
    throw std::runtime_error("failed to check dimnames at '" + ritsuko::hdf5::get_name(handle) + "'; " + std::string(e.what()));
}

template<class Dimensions_>
void check_dimnames(const H5::H5File& handle, const std::string& dimnames_group, const Dimensions_& dimensions) try {
    if (!handle.exists(dimnames_group) || handle.childObjType(dimnames_group) != H5O_TYPE_GROUP) {
        throw std::runtime_error("expected a group");
    }
    auto nhandle = handle.openGroup(dimnames_group);

    for (size_t i = 0, ndim = dimensions.size(); i < ndim; ++i) {
        std::string dim_name = std::to_string(i);
        if (!nhandle.exists(dim_name)) {
            continue;
        }

        if (nhandle.childObjType(dim_name) != H5O_TYPE_DATASET) {
            throw std::runtime_error("expected '" + dim_name + "' to be a dataset");
        }
        check_dimnames(nhandle.openDataSet(dim_name), dimensions[i]);
    }
} catch (std::exception& e) {
    throw std::runtime_error("failed to validate dimnames at '" + dimnames_group + "'; " + std::string(e.what()));
}

template<class Host_, class Dimensions_>
void check_dimnames2(const H5::H5File& handle, const Host_& host, const Dimensions_& dimensions, bool reverse = false) try {
    auto dimnames = host.openAttribute("dimnames-group");
    if (dimnames.getTypeClass() != H5T_STRING) {
        throw std::runtime_error("'" + dimnames.getName() + "' attribute should have a string datatype");
    }
    
    size_t ndim = dimensions.size();
    if (ritsuko::hdf5::get_1d_length(dimnames.getSpace(), false) != ndim) {
        throw std::runtime_error("'" + dimnames.getName() + "' attribute should have length equal to the number of dimensions (" + std::to_string(ndim) + ")");
    }

    ritsuko::hdf5::load_1d_string_attribute(
        dimnames, 
        dimensions.size(), 
        [&](size_t i, const char* start, size_t len) {
            if (len) {
                auto x = std::string(start, start + len); // need to allocate to ensure correct null termination.
                auto dhandle = ritsuko::hdf5::get_dataset(handle, x.c_str());
                check_dimnames(dhandle, dimensions[reverse ? ndim - i - 1 : i]);
            }
        }
    );
} catch (std::exception& e) {
    throw std::runtime_error("failed to validate dimnames for '" + ritsuko::hdf5::get_name(host) + "'; " + std::string(e.what()));
}
/**
 * @endcond
 */
}

}

#endif
