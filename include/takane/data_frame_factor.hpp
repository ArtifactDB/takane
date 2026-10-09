#ifndef TAKANE_DATA_FRAME_FACTOR_HPP
#define TAKANE_DATA_FRAME_FACTOR_HPP

#include <string>
#include <stdexcept>
#include <filesystem>

#include "H5Cpp.h"
#include "ritsuko/ritsuko.hpp"
#include "sanisizer/sanisizer.hpp"

#include "utils_public.hpp"
#include "utils_string.hpp"
#include "utils_factor.hpp"
#include "utils_json.hpp"
#include "utils_other.hpp"

/**
 * @file data_frame_factor.hpp
 * @brief Validation for data frame factors.
 */

namespace takane {

/**
 * @cond
 */
void validate(const std::filesystem::path&, const ObjectMetadata&, const Options&);
std::size_t height(const std::filesystem::path&, const ObjectMetadata&, const Options&);
bool satisfies_interface(const std::string&, const std::string&, const Options&);
/**
 * @endcond
 */

/**
 * If `Options::data_frame_factor_any_duplicated` is set, it enables stricter checking of the uniqueness of the data frame levels.
 * Currently, we don't provide a default method for `data_frame` objects, as it's kind of tedious and we haven't gotten around to it yet.
 *
 * @param path Path to the directory containing the data frame factor.
 * @param metadata Metadata for the object, typically read from its `OBJECT` file.
 * @param options Validation options.
 */
inline void validate_data_frame_factor(const std::filesystem::path& path, const ObjectMetadata& metadata, const Options& options) {
    const std::string type_name = "data_frame_factor"; // use a separate variable to avoid dangling reference warnings from GCC.

    ritsuko::Version version;
    try {
        const auto& type_meta = extract_json_object(metadata.other, type_name);
        try {
            const auto& vstring = extract_json_version_string(type_meta);
            version = ritsuko::parse_version_string(vstring.c_str(), vstring.size(), /* skip_patch = */ true);
            if (version.major != 1) {
                throw std::runtime_error("unsupported version string '" + vstring + "'");
            }
        } catch (...) {
            std::throw_with_nested(std::runtime_error("failed to validate 'version'"));
        }
    } catch (...) {
        std::throw_with_nested(std::runtime_error("failed to validate '" + type_name + "' in the object metadata"));
    }

    std::size_t num_levels;
    try {
        auto lpath = path / "levels";
        auto lmeta = read_object_metadata(lpath);
        if (!satisfies_interface(lmeta.type, "DATA_FRAME", options)) {
            throw std::runtime_error("expected 'levels' to be an object that satisfies the 'DATA_FRAME' interface");
        }
        validate(lpath, lmeta, options);

        if (options.data_frame_factor_any_duplicated) {
            if (options.data_frame_factor_any_duplicated(lpath, lmeta, options)) {
                throw std::runtime_error("'levels' should not contain duplicated rows");
            }
        }

        num_levels = height(lpath, lmeta, options);
    } catch (...) {
        std::throw_with_nested(std::runtime_error("failed to validate 'levels'"));
    }

    hsize_t num_codes;
    try {
        H5::H5File handle(path / "contents.h5", H5F_ACC_RDONLY);
        auto ghandle = handle.openGroup(type_name);
        try {
            num_codes = validate_factor_codes(ghandle.openDataSet("codes"), num_levels, options.hdf5_buffer_size, /* allow_missing = */ false);
        } catch (...) {
            std::throw_with_nested(std::runtime_error("failed to validate 'codes'"));
        }
        validate_names(ghandle, "names", num_codes, options.hdf5_buffer_size);
    } catch (...) {
        std::throw_with_nested(std::runtime_error("failed to validate '" + type_name + "' in 'contents.h5'"));
    }

    validate_mcols(path, "element_annotations", num_codes, options);
    validate_metadata(path, "other_annotations", options);
}

/**
 * @param path Path to the directory containing the data frame factor.
 * @param metadata Metadata for the object, typically read from its `OBJECT` file.
 * @param options Validation options.
 * @return Length of the factor.
 */
inline std::size_t height_of_data_frame_factor(const std::filesystem::path& path, [[maybe_unused]] const ObjectMetadata& metadata, [[maybe_unused]] const Options& options) {
    H5::H5File handle(path / "contents.h5", H5F_ACC_RDONLY);
    auto ghandle = handle.openGroup("data_frame_factor");
    auto dhandle = ghandle.openDataSet("codes");
    hsize_t output;
    dhandle.getSpace().getSimpleExtentDims(&output);
    return sanisizer::cast<std::size_t>(output);
}

}

#endif
