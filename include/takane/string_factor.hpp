#ifndef TAKANE_STRING_FACTOR_HPP
#define TAKANE_STRING_FACTOR_HPP

#include <string>
#include <stdexcept>
#include <filesystem>

#include "ritsuko/hdf5/hdf5.hpp"

#include "utils_public.hpp"
#include "utils_hdf5.hpp"

/**
 * @file string_factor.hpp
 * @brief Validation for string factors.
 */

namespace takane {

/**
 * @namespace takane::string_factor
 * @brief Definitions for string factors.
 */
namespace string_factor {

/**
 * @param path Path to the directory containing the string factor.
 * @param options Validation options, typically for reading performance.
 */
inline void validate(const std::filesystem::path& path, const Options& options) try {
    auto handle = ritsuko::hdf5::open_file(path / "contents.h5");
    auto ghandle = ritsuko::hdf5::open_group(handle, "string_factor");

    auto vstring = internal_hdf5::open_and_load_scalar_string_attribute(ghandle, "version");
    auto version = ritsuko::parse_version_string(vstring.c_str(), vstring.size(), /* skip_patch = */ true);
    if (version.major != 1) {
        throw std::runtime_error("unsupported version string '" + vstring + "'");
    }

    internal_hdf5::check_ordered_attribute(ghandle);

    size_t num_levels = internal_hdf5::validate_factor_levels(ghandle, "levels", options.hdf5_buffer_size);
    size_t num_codes = internal_hdf5::validate_factor_codes(ghandle, "codes", num_levels, options.hdf5_buffer_size);

    if (ghandle.exists("names")) {
        auto nhandle = ritsuko::hdf5::open_dataset(ghandle, "names");
        if (nhandle.getTypeClass() != H5T_STRING) {
            throw std::runtime_error("'names' should be a string datatype class");
        }
        auto nlen = ritsuko::hdf5::get_1d_length(nhandle.getSpace(), false);
        if (num_codes != nlen) {
            throw std::runtime_error("'names' and 'codes' should have the same length");
        }
    }

} catch (std::exception& e) {
    throw std::runtime_error("failed to validate a 'string_factor' at '" + path.string() + "'; " + std::string(e.what()));
}

/**
 * @param path Path to the directory containing the string factor.
 * @param options Validation options, typically for reading performance.
 * @return Length of the factor.
 */
inline size_t height(const std::filesystem::path& path, const Options&) {
    H5::H5File handle((path / "contents.h5").string(), H5F_ACC_RDONLY);
    auto ghandle = handle.openGroup("string_factor");
    auto dhandle = ghandle.openDataSet("codes");
    return ritsuko::hdf5::get_1d_length(dhandle.getSpace(), false);
}

}

}

#endif
