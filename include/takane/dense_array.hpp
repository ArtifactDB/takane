#ifndef TAKANE_DENSE_ARRAY_HPP
#define TAKANE_DENSE_ARRAY_HPP

#include "H5Cpp.h"
#include "ritsuko/ritsuko.hpp"
#include "sanisizer/sanisizer.hpp"

#include "utils_public.hpp"
#include "utils_array.hpp"
#include "utils_json.hpp"
#include "utils_missing.hpp"
#include "utils_string.hpp"

#include <vector>
#include <string>
#include <stdexcept>
#include <filesystem>
#include <cstdint>

/**
 * @file dense_array.hpp
 * @brief Validation for dense arrays.
 */

namespace takane {

/**
 * @param path Path to the directory containing a dense array.
 * @param metadata Metadata for the object, typically read from its `OBJECT` file.
 * @param options Validation options.
 */
inline void validate_dense_array(const std::filesystem::path& path, const ObjectMetadata& metadata, const Options& options) {
    const std::string type_name = "dense_array"; // use a separate variable to avoid dangling reference warnings from GCC.

    const auto& type_meta = extract_json_type_metadata(metadata.other, type_name);
    const auto& vstring = extract_json_version_string(type_meta, type_name);
    auto version = ritsuko::parse_version_string(vstring.c_str(), vstring.size(), /* skip_patch = */ true);
    if (version.major != 1) {
        throw std::runtime_error("unsupported version '" + vstring + "'");
    }

    H5::H5File handle(path / "array.h5", H5F_ACC_RDONLY);
    auto ghandle = handle.openGroup(type_name);

    if (ghandle.attrExists("transposed")) {
        auto ahandle = ghandle.openAttribute("transposed");
        if (ahandle.getSpace().getSimpleExtentNdims() != 0) {
            throw std::runtime_error("expected 'transposed' attribute to be a scalar");
        }
        if (ritsuko::hdf5::exceeds_integer_limit(ahandle, 32, true)) {
            throw std::runtime_error("expected 'transposed' attribute to have a datatype that fits in a 32-bit signed integer");
        }
    }

    auto type = open_and_load_scalar_string_attribute(ghandle, "type");
    const char* missing_attr_name = "missing-value-placeholder";
    std::vector<hsize_t> extents;

    if (type == "vls") {
        if (version.lt(1, 1, 0)) {
            throw std::runtime_error("unsupported type '" + type + "'");
        }

        auto hhandle = ghandle.openDataSet("heap");
        const auto hlen = ritsuko::cvls::validate_heap(hhandle);

        auto phandle = ghandle.openDataSet("pointers");
        auto pspace = phandle.getSpace();
        auto ndim = pspace.getSimpleExtentNdims();
        if (ndim == 0) {
            throw std::runtime_error("expected 'pointers' to have at least one dimension");
        }
        sanisizer::resize(extents, ndim);
        pspace.getSimpleExtentDims(extents.data());

        ritsuko::cvls::validate_nd_pointers<std::uint64_t, std::uint64_t>(
            phandle,
            extents,
            hlen,
            [&]{
                ritsuko::cvls::ValidateNdPointersOptions opt;
                opt.contiguous_chunk_size = options.hdf5_buffer_size;
                return opt;
            }()
        );
        validate_string_missing_placeholder(phandle, missing_attr_name);

    } else {
        auto dhandle = ghandle.openDataSet("data");
        auto dspace = dhandle.getSpace();
        auto ndim = dspace.getSimpleExtentNdims();
        if (ndim == 0) {
            throw std::runtime_error("expected 'data' to have at least one dimension");
        }
        sanisizer::resize(extents, ndim);
        dspace.getSimpleExtentDims(extents.data());

        if (type == "string") {
            if (!ritsuko::hdf5::is_utf8_string(dhandle)) {
                throw std::runtime_error("expected string array to have a datatype that can be represented by a UTF-8 encoded string");
            }

            ritsuko::hdf5::validate_nd_strings(
                dhandle,
                extents,
                [&]{
                    ritsuko::hdf5::ValidateNdStringsOptions opt;
                    opt.contiguous_chunk_size = options.hdf5_buffer_size;
                    return opt;
                }()
            );
            validate_string_missing_placeholder(dhandle, missing_attr_name);

        } else {
            if (type == "integer") {
                if (ritsuko::hdf5::exceeds_integer_limit(dhandle, 32, true)) {
                    throw std::runtime_error("expected integer array to have a datatype that fits into a 32-bit signed integer");
                }
            } else if (type == "boolean") {
                if (ritsuko::hdf5::exceeds_integer_limit(dhandle, 32, true)) {
                    throw std::runtime_error("expected boolean array to have a datatype that fits into a 32-bit signed integer");
                }
            } else if (type == "number") {
                if (ritsuko::hdf5::exceeds_float_limit(dhandle, 64)) {
                    throw std::runtime_error("expected number array to have a datatype that fits into a 64-bit float");
                }
            } else {
                throw std::runtime_error("unknown array type '" + type + "'");
            }

            validate_numeric_missing_placeholder(dhandle, missing_attr_name);
        }
    }

    validate_array_dimnames(ghandle, "names", extents, options);
}

/**
 * @param path Path to the directory containing a dense array.
 * @param metadata Metadata for the object, typically read from its `OBJECT` file.
 * @param options Validation options.
 * @return Extent of the first dimension.
 */
inline std::size_t height_of_dense_array(const std::filesystem::path& path, [[maybe_unused]] const ObjectMetadata& metadata, [[maybe_unused]] const Options& options) {
    H5::H5File handle(path / "array.h5", H5F_ACC_RDONLY);
    auto ghandle = handle.openGroup("dense_array");
    auto type = open_and_load_scalar_string_attribute(ghandle, "type");

    H5::DataSpace space;
    if (type == "vls") {
        space = ghandle.openDataSet("pointers").getSpace();
    } else {
        space = ghandle.openDataSet("data").getSpace();
    }
    auto ndims = space.getSimpleExtentNdims();
    auto extents = sanisizer::create<std::vector<hsize_t> >(ndims);
    space.getSimpleExtentDims(extents.data());

    std::int32_t transposed = 0;
    if (ghandle.attrExists("transposed")) {
        auto ahandle = ghandle.openAttribute("transposed");
        ahandle.read(H5::PredType::NATIVE_INT32, &transposed);
    }

    return sanisizer::cast<std::size_t>(transposed ? extents.back() : extents.front());
}

/**
 * @param path Path to the directory containing a dense array.
 * @param metadata Metadata for the object, typically read from its `OBJECT` file.
 * @param options Validation options.
 * @return Dimensions of the array.
 */
inline std::vector<std::size_t> dimensions_of_dense_array(const std::filesystem::path& path, [[maybe_unused]] const ObjectMetadata& metadata, [[maybe_unused]] const Options& options) {
    H5::H5File handle(path / "array.h5", H5F_ACC_RDONLY);
    auto ghandle = handle.openGroup("dense_array");
    auto type = open_and_load_scalar_string_attribute(ghandle, "type");

    H5::DataSpace space;
    if (type == "vls") {
        space = ghandle.openDataSet("pointers").getSpace();
    } else {
        space = ghandle.openDataSet("data").getSpace();
    }

    auto ndims = space.getSimpleExtentNdims();
    auto extents = sanisizer::create<std::vector<hsize_t> >(ndims);
    space.getSimpleExtentDims(extents.data());

    std::int32_t transposed = 0;
    if (ghandle.attrExists("transposed")) {
        auto ahandle = ghandle.openAttribute("transposed");
        ahandle.read(H5::PredType::NATIVE_INT32, &transposed);
        if (transposed) {
            std::reverse(extents.begin(), extents.end());
        }
    }

    return cast_array_dimensions<std::size_t>(std::move(extents));
}

}

#endif
