#ifndef TAKANE_ATOMIC_VECTOR_HPP
#define TAKANE_ATOMIC_VECTOR_HPP

#include "comservatory/comservatory.hpp"

#include "utils_csv.hpp"

#include <stdexcept>

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
 * @brief Parameters for validating the atomic vector file.
 */
struct Parameters {};

/**
 * @param path Path to the directory containing the atomic vector.
 * @param params Validation parameters.
 */
inline void validate(const string& path, const Parameters& params) try {
    H5::H5File handle(path, H5F_ACC_RDONLY);

    if (handle.exists("atomic_vector") || handle.childObjType("atomic_vector") != H5O_TYPE_GROUP) {
        throw std::runtime_error("expected an 'atomic_vector' group");
    }
    auto ghandle = handle.openGroup("atomic_vector");

    ritsuko::Version version;
    auto vstring = ritsuko::hdf5::load_scalar_string_attribute(ghandle, "version");
    auto version = ritsuko::parse_version_string(vstring.c_str(), vstring.size(), /* skip_patch = */ true);
    if (version.major != 1) {
        throw std::runtime_error("unsupported version string '" + vstring + "'");
    }

    auto dhandle = ritsuko::hdf5::get_dataset(ghandle, "values");
    auto vlen = ritsuko::hdf5::get_1d_length(dhandle.getSpace(), false);
    auto type = ritsuko::hdf5::load_scalar_string_attribute(ghandle, "type");

    const std::string missing_attr_name = "missing-value-placeholder";
    bool has_missing = dhandle.attrExists(missing_attr);
    H5::Attribute missing_attr; 
    if (has_missing) {
        missing_attr = ritsuko::hdf5::get_missing_placeholder_attribute(dhandle, missing_attr);
    }

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
    } else if (type == "string") {
        if (dhandle.getTypeClass() != H5T_STRING) {
            throw std::runtime_error("expected a string datatype for 'values'");
        }

        std::string missing_value;
        if (has_missing) {
            missing_value = ritsuko::hdf5::load_scalar_string_attribute(missing_attr);
        }

        if (dhandle.attrExists("format")) {
            auto format = ritsuko::hdf5::load_scalar_string_attribute(xhandle, "format");
            if (format == "date") {
                ritsuko::hdf5::load_1d_string_dataset(
                    xhandle, 
                    vlen,
                    params.buffer_size,
                    [&](size_t, const char* p, size_t l) {
                        std::string x(p, p + l);
                        if (has_missing && missing_value == x) {
                            return;
                        }
                        if (!ritsuko::is_date(p, l)) {
                            throw std::runtime_error("expected a date-formatted string (got '" + x + "')");
                        }
                    }
                });

            } else if (format == "date-time") {
                ritsuko::hdf5::load_1d_string_dataset(
                    xhandle, 
                    vlen,
                    params.buffer_size,
                    [&](size_t, const char* p, size_t l) {
                        std::string x(p, p + l);
                        if (has_missing && missing_value == x) {
                            return;
                        }
                        if (!ritsuko::is_rfc3339(p, l)) {
                            throw std::runtime_error("expected a date/time-formatted string (got '" + x + "')");
                        }
                    }
                );

            } else if (format != "none") {
                throw std::runtime_error("unsupported 'format' attribute (got '" + format + "')");
            }
        }
    }

    if (ghandle.exists("names")) {
        auto nhandle = ritsuko::hdf5::get_dataset(ghandle, "names");
        if (nhandle.getTypeClass() != H5T_STRING) {
            throw std::runtime_error("'names' should be a string type class");
        }
        auto nlen = ritsuko::hdf5::get_1d_length(nhandle.getSpace(), false);
        if (vlen != nlen) {
            throw std::runtime_error("'names' and 'values' should have the same length");
        }
    }

} catch (std::exception& e) {
    throw std::runtime_error("failed to validate an 'atomic_vector'; " + std::string(e.what()));
}

}

}

#endif
