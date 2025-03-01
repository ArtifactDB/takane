#ifndef TAKANE_ATOMIC_VECTOR_HPP
#define TAKANE_ATOMIC_VECTOR_HPP

#include <string>
#include <stdexcept>
#include <filesystem>

#include "ritsuko/hdf5/hdf5.hpp"
#include "ritsuko/hdf5/vls/vls.hpp"

#include "utils_public.hpp"
#include "utils_string.hpp"
#include "utils_json.hpp"

/**
 * @file atomic_vector.hpp
 * @brief Validation for atomic vectors.
 */

namespace takane {

/**
 * @namespace takane::atomic_vector
 * @brief Definitions for atomic vectors.
 */
namespace atomic_vector {

/**
 * @param path Path to the directory containing the atomic vector.
 * @param metadata Metadata for the object, typically read from its `OBJECT` file.
 * @param options Validation options.
 */
inline void validate(const std::filesystem::path& path, const ObjectMetadata& metadata, Options& options) {
    const std::string type_name = "atomic_vector"; // use a separate variable to avoid dangling reference warnings from GCC.
    const auto& vstring = internal_json::extract_version_for_type(metadata.other, type_name);
    auto version = ritsuko::parse_version_string(vstring.c_str(), vstring.size(), /* skip_patch = */ true);
    if (version.major != 1) {
        throw std::runtime_error("unsupported version string '" + vstring + "'");
    }

    auto handle = ritsuko::hdf5::open_file(path / "contents.h5");
    auto ghandle = ritsuko::hdf5::open_group(handle, type_name.c_str());
    auto type = ritsuko::hdf5::open_and_load_scalar_string_attribute(ghandle, "type");
    hsize_t vlen = 0;

    const char* missing_attr_name = "missing-value-placeholder";

    if (type == "vls") {
        if (version.lt(1, 1, 0)) {
            throw std::runtime_error("unsupported type '" + type + "'");
        }

        auto phandle = ritsuko::hdf5::vls::open_pointers(ghandle, "pointers", 64, 64);
        vlen = ritsuko::hdf5::get_1d_length(phandle.getSpace(), false);
        auto hhandle = ritsuko::hdf5::vls::open_heap(ghandle, "heap");
        auto hlen = ritsuko::hdf5::get_1d_length(hhandle.getSpace(), false);
        ritsuko::hdf5::vls::validate_1d_array<uint64_t, uint64_t>(phandle, vlen, hlen, options.hdf5_buffer_size);

        if (phandle.attrExists(missing_attr_name)) {
            auto attr = phandle.openAttribute(missing_attr_name);
            ritsuko::hdf5::check_string_missing_placeholder_attribute(attr);
        }

    } else {
        auto dhandle = ritsuko::hdf5::open_dataset(ghandle, "values");
        vlen = ritsuko::hdf5::get_1d_length(dhandle.getSpace(), false);

        if (type == "string") {
            if (!ritsuko::hdf5::is_utf8_string(dhandle)) {
                throw std::runtime_error("expected a datatype for 'values' that can be represented by a UTF-8 encoded string");
            }
            auto missingness = ritsuko::hdf5::open_and_load_optional_string_missing_placeholder(dhandle, missing_attr_name);
            std::string format = internal_string::fetch_format_attribute(ghandle);
            internal_string::validate_string_format(dhandle, vlen, format, missingness, options.hdf5_buffer_size);

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

            if (dhandle.attrExists(missing_attr_name)) {
                auto missing_attr = dhandle.openAttribute(missing_attr_name);
                ritsuko::hdf5::check_numeric_missing_placeholder_attribute(dhandle, missing_attr);
            }
        }
    }

    internal_string::validate_names(ghandle, "names", vlen, options.hdf5_buffer_size);
}

/**
 * @param path Path to the directory containing the atomic vector.
 * @param metadata Metadata for the object, typically read from its `OBJECT` file.
 * @param options Validation options.
 * @return Length of the vector.
 */
inline size_t height(const std::filesystem::path& path, [[maybe_unused]] const ObjectMetadata& metadata, [[maybe_unused]] Options& options) {
    auto handle = ritsuko::hdf5::open_file(path / "contents.h5");
    auto ghandle = handle.openGroup("atomic_vector");
    auto type = ritsuko::hdf5::open_and_load_scalar_string_attribute(ghandle, "type");

    if (type == "vls") {
        auto phandle = ghandle.openDataSet("pointers");
        return ritsuko::hdf5::get_1d_length(phandle.getSpace(), false);
    } else {
        auto dhandle = ghandle.openDataSet("values");
        return ritsuko::hdf5::get_1d_length(dhandle.getSpace(), false);
    }
}

}

}

#endif
