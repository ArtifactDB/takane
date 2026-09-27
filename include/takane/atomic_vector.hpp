#ifndef TAKANE_ATOMIC_VECTOR_HPP
#define TAKANE_ATOMIC_VECTOR_HPP

#include <string>
#include <stdexcept>
#include <filesystem>

#include "H5Cpp.h"
#include "ritsuko/ritsuko.hpp"
#include "sanisizer/sanisizer.hpp"

#include "utils_public.hpp"
#include "utils_string.hpp"
#include "utils_json.hpp"
#include "utils_missing.hpp"
#include "utils_other.hpp"

/**
 * @file atomic_vector.hpp
 * @brief Validation for atomic vectors.
 */

namespace takane {

/**
 * @param path Path to the directory containing the atomic vector.
 * @param metadata Metadata for the object, typically read from its `OBJECT` file.
 * @param options Validation options.
 */
inline void validate_atomic_vector(const std::filesystem::path& path, const ObjectMetadata& metadata, const Options& options) {
    const std::string type_name = "atomic_vector"; // use a separate variable to avoid dangling reference warnings from GCC.
    const auto& vstring = extract_version_for_type(metadata.other, type_name);
    auto version = ritsuko::parse_version_string(vstring.c_str(), vstring.size(), /* skip_patch = */ true);
    if (version.major != 1) {
        throw std::runtime_error("unsupported version string '" + vstring + "'");
    }

    H5::H5File handle(path / "contents.h5", H5F_ACC_RDONLY);
    auto ghandle = handle.openGroup(type_name);
    auto type = open_and_load_scalar_string_attribute(ghandle, "type");
    hsize_t vlen = 0;

    const char* missing_attr_name = "missing-value-placeholder";

    if (type == "vls") {
        if (version.lt(1, 1, 0)) {
            throw std::runtime_error("unsupported type '" + type + "'");
        }

        auto hhandle = ghandle.openDataSet("heap");
        const auto hlen = ritsuko::cvls::validate_heap(hhandle);

        auto phandle = ghandle.openDataSet("pointers");
        auto pspace = phandle.getSpace();
        if (pspace.getSimpleExtentNdims() != 1) {
            throw std::runtime_error("expected 'pointers' to be a 1-dimensional dataset");
        }
        pspace.getSimpleExtentDims(&vlen);
        ritsuko::cvls::validate_1d_pointers<std::uint64_t, std::uint64_t>(
            phandle,
            vlen,
            hlen,
            [&]{
                ritsuko::cvls::Validate1dPointersOptions opt;
                opt.contiguous_chunk_size = options.hdf5_buffer_size;
                return opt;
            }()
        );

        check_string_missing_placeholder(phandle, missing_attr_name);

    } else {
        auto dhandle = ghandle.openDataSet("values");
        auto dspace = dhandle.getSpace();
        if (dspace.getSimpleExtentNdims() != 1) {
            throw std::runtime_error("expected 'values' to be a 1-dimensional dataset");
        }
        dspace.getSimpleExtentDims(&vlen);

        if (type == "string") {
            if (!ritsuko::hdf5::is_utf8_string(dhandle)) {
                throw std::runtime_error("expected a datatype for 'values' that can be represented by a UTF-8 encoded string");
            }
            auto missingness = read_string_missing_placeholder(dhandle, missing_attr_name);
            const auto format = open_and_load_string_format(ghandle);
            validate_string_format(dhandle, vlen, format, missingness, options.hdf5_buffer_size);

        } else {
            if (type == "integer") {
                if (ritsuko::hdf5::exceeds_integer_limit(dhandle, 32, true)) {
                    throw std::runtime_error("expected a datatype for 'values' that fits in a 32-bit signed integer");
                }
            } else if (type == "boolean") {
                if (ritsuko::hdf5::exceeds_integer_limit(dhandle, 32, true)) {
                    throw std::runtime_error("expected a datatype for 'values' that fits in a 32-bit signed integer");
                }
            } else if (type == "number") {
                if (ritsuko::hdf5::exceeds_float_limit(dhandle, 64)) {
                    throw std::runtime_error("expected a datatype for 'values' that fits in a 64-bit float");
                }
            } else {
                throw std::runtime_error("unsupported type '" + type + "'");
            }

            check_numeric_missing_placeholder(dhandle, missing_attr_name);
        }
    }

    validate_names(ghandle, "names", vlen, options.hdf5_buffer_size);
}

/**
 * @param path Path to the directory containing the atomic vector.
 * @param metadata Metadata for the object, typically read from its `OBJECT` file.
 * @param options Validation options.
 * @return Length of the vector.
 */
inline std::size_t height_of_atomic_vector(const std::filesystem::path& path, [[maybe_unused]] const ObjectMetadata& metadata, [[maybe_unused]] const Options& options) {
    H5::H5File handle(path / "contents.h5", H5F_ACC_RDONLY);
    auto ghandle = handle.openGroup("atomic_vector");
    auto type = open_and_load_scalar_string_attribute(ghandle, "type");

    hsize_t len;
    if (type == "vls") {
        auto phandle = ghandle.openDataSet("pointers");
        phandle.getSpace().getSimpleExtentDims(&len);
    } else {
        auto dhandle = ghandle.openDataSet("values");
        dhandle.getSpace().getSimpleExtentDims(&len);
    }

    return sanisizer::cast<std::size_t>(len);
}

}

#endif
