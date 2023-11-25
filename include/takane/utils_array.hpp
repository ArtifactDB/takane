#ifndef TAKANE_ARRAY_HPP
#define TAKANE_ARRAY_HPP

#include "H5Cpp.h"
#include "ritsuko/hdf5/hdf5.hpp"
#include "ritsuko/ritsuko.hpp"
#include "utils_public.hpp"

#include <string>
#include <vector>
#include <stdexcept>

namespace takane {

namespace internal_array {

template<class Size_>
void check_dimnames(const H5::Group& handle, const std::string& name, const std::vector<Size_>& dimensions, const Options& options) try {
    if (handle.childObjType(name) != H5O_TYPE_GROUP) {
        throw std::runtime_error("expected '" + name + "' to be a group");
    }
    auto nhandle = handle.openGroup(name);

    for (size_t d = 0, ndim = dimensions.size(); d < ndim; ++d) {
        std::string dname = std::to_string(d);
        if (!nhandle.exists(dname)) {
            continue;
        }

        if (nhandle.childObjType(dname) != H5O_TYPE_DATASET) {
            throw std::runtime_error("expected '" + name + "/" + dname + "' to be a dataset");
        }
        auto dhandle = nhandle.openDataSet(dname);

        auto len = ritsuko::hdf5::get_1d_length(dhandle, false);
        if (len != dimensions[d]) {
            throw std::runtime_error("expected '" + name + "/" + dname + 
                "' to have the same length as the extent of the corresponding dimension (got " +
                std::to_string(len) + ", expected " + std::to_string(dimensions[d]) + ")"); 
        }

        if (dhandle.getTypeClass() != H5T_STRING) {
            throw std::runtime_error("expected '" + name + "/" + dname + "' to have a string datatype class");
        }

        ritsuko::hdf5::validate_1d_string_dataset(dhandle, len, options.hdf5_buffer_size);
    }
} catch (std::exception& e) {
    throw std::runtime_error("failed to validate dimnames for '" + ritsuko::hdf5::get_name(handle) + "'; " + std::string(e.what()));
}
/**
 * @endcond
 */
}

}

#endif
