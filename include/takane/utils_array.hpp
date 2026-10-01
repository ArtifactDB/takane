#ifndef TAKANE_ARRAY_HPP
#define TAKANE_ARRAY_HPP

#include "H5Cpp.h"
#include "ritsuko/ritsuko.hpp"
#include "sanisizer/sanisizer.hpp"

#include "utils_public.hpp"
#include "utils_other.hpp"

#include <string>
#include <vector>
#include <stdexcept>
#include <type_traits>

namespace takane {

template<typename Output_, typename Input_>
std::vector<Output_> cast_array_dimensions(std::vector<Input_> input) {
    if constexpr(std::is_same<Input_, Output_>::value) {
        return input;
    } else {
        const auto ndim = input.size();
        auto output = sanisizer::create<std::vector<Output_> >(ndim);
        for (I<decltype(ndim)> d = 0; d < ndim; ++d) {
            output[d] = sanisizer::cast<Output_>(input[d]);
        }
        return output;
    }
}

template<class Size_>
void validate_array_dimnames(const H5::Group& handle, const std::string& name, const std::vector<Size_>& dimensions, const Options& options) try {
    if (!handle.exists(name)) {
        return;
    }

    auto nhandle = handle.openGroup(name);
    const auto ndim = dimensions.size();
    auto found = ndim;

    for (I<decltype(ndim)> d = 0; d < ndim; ++d) {
        std::string dname = std::to_string(d);
        if (!nhandle.exists(dname)) {
            --found;
            continue;
        }

        auto dhandle = nhandle.openDataSet(dname);
        auto dspace = dhandle.getSpace(); 
        if (dspace.getSimpleExtentNdims() != 1) {
            throw std::runtime_error("expected the '" + dname + "' dataset to be 1-dimensional");
        }
        hsize_t len;
        dspace.getSimpleExtentDims(&len);

        if (!ritsuko::hdf5::is_utf8_string(dhandle)) {
            throw std::runtime_error("expected the '" + dname + "' dataset to contain UTF-8 encoded strings");
        }

        if (!sanisizer::is_equal(len, dimensions[d])) {
            throw std::runtime_error("expected the '" + dname + "' dataset to have the same length as the extent of the corresponding array dimension");
        }

        ritsuko::hdf5::validate_1d_strings(
            dhandle,
            len,
            [&]{
                ritsuko::hdf5::Validate1dStringsOptions opt;
                opt.contiguous_chunk_size = options.hdf5_buffer_size;
                return opt;
            }()
        );
    }

    if (!sanisizer::is_equal(found, nhandle.getNumObjs())) {
        throw std::runtime_error("more objects present in the group than expected");
    }

} catch (std::exception& e) {
    throw std::runtime_error("failed to validate '" + name + "'; " + std::string(e.what()));
}

}

#endif
