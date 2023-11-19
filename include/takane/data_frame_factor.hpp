#ifndef TAKANE_DATA_FRAME_FACTOR_HPP
#define TAKANE_DATA_FRAME_FACTOR_HPP

#include <string>
#include <stdexcept>
#include <filesystem>

#include "ritsuko/hdf5/hdf5.hpp"

#include "utils_public.hpp"
#include "utils_hdf5.hpp"

/**
 * @file data_frame_factor.hpp
 * @brief Validation for data frame factors.
 */

namespace takane {

/**
 * @namespace takane::data_frame_factor
 * @brief Definitions for data frame factors.
 */
namespace data_frame_factor {

/**
 * @param path Path to the directory containing the data frame factor.
 * @param options Validation options, typically for reading performance.
 */
inline void validate(const std::filesystem::path& path, const Options& options) try {
    H5::H5File handle(path / "contents.h5", H5F_ACC_RDONLY);

    const char* parent = "data_frame_factor";
    if (!handle.exists(parent) || handle.childObjType(parent) != H5O_TYPE_GROUP) {
        throw std::runtime_error("expected a 'data_frame_factor' group");
    }
    auto ghandle = handle.openGroup(parent);

    auto vstring = ritsuko::hdf5::load_scalar_string_attribute(ghandle, "version");
    auto version = ritsuko::parse_version_string(vstring.c_str(), vstring.size(), /* skip_patch = */ true);
    if (version.major != 1) {
        throw std::runtime_error("unsupported version string '" + vstring + "'");
    }

    // Validating the levels.
    auto lpath = path / "levels";
    try {
        ::takane::validate(lpath, options);
    } catch (std::exception& e) {
        throw std::runtime_error("failed to validate 'levels'; " + std::string(e.what()));
    }
    size_t num_levels = ::takane::height(lpath, options);
    size_t num_codes = internal_hdf5::validate_factor_codes(ghandle, "codes", num_levels, options.hdf5_buffer_size, /* allow_missing = */ false);

    if (ghandle.exists("names")) {
        auto nhandle = ritsuko::hdf5::get_dataset(ghandle, "names");
        if (nhandle.getTypeClass() != H5T_STRING) {
            throw std::runtime_error("'names' should be a string datatype class");
        }
        auto nlen = ritsuko::hdf5::get_1d_length(nhandle.getSpace(), false);
        if (num_codes != nlen) {
            throw std::runtime_error("'names' and 'codes' should have the same length");
        }
    }

    // Checking the metadata.
    try {
        internal_other::validate_mcols(path / "element_annotations", num_codes, options);
    } catch (std::exception& e) {
        throw std::runtime_error("failed to validate 'element_annotations'; " + std::string(e.what()));
    }

    try {
        internal_other::validate_metadata(path / "other_annotations", options);
    } catch (std::exception& e) {
        throw std::runtime_error("failed to validate 'other_annotations'; " + std::string(e.what()));
    }

} catch (std::exception& e) {
    throw std::runtime_error("failed to validate a 'data_frame_factor' at '" + path.string() + "'; " + std::string(e.what()));
}

/**
 * @param path Path to the directory containing the data frame factor.
 * @param options Validation options, typically for reading performance.
 * @return Length of the factor.
 */
inline size_t height(const std::filesystem::path& path, const Options&) {
    H5::H5File handle(path / "contents.h5", H5F_ACC_RDONLY);
    auto ghandle = handle.openGroup("data_frame_factor");
    auto dhandle = ghandle.openDataSet("codes");
    return ritsuko::hdf5::get_1d_length(dhandle.getSpace(), false);
}

}

}

#endif
