#ifndef TAKANE_STRING_FACTOR_HPP
#define TAKANE_STRING_FACTOR_HPP

#include <string>
#include <stdexcept>
#include <filesystem>

#include "H5Cpp.h"
#include "ritsuko/ritsuko.hpp"

#include "utils_public.hpp"
#include "utils_string.hpp"
#include "utils_factor.hpp"

/**
 * @file string_factor.hpp
 * @brief Validation for string factors.
 */

namespace takane {

/**
 * @param path Path to the directory containing the string factor.
 * @param metadata Metadata for the object, typically read from its `OBJECT` file.
 * @param options Validation options.
 */
inline void validate_string_factor(const std::filesystem::path& path, const ObjectMetadata& metadata, const Options& options) {
    const std::string type_name = "string_factor"; // use a separate variable to avoid dangling reference warnings from GCC.

    const auto& type_meta = extract_json_type_metadata(metadata.other, type_name);
    const auto& vstring = extract_json_version_string(type_meta, type_name);
    auto version = ritsuko::parse_version_string(vstring.c_str(), vstring.size(), /* skip_patch = */ true);
    if (version.major != 1) {
        throw std::runtime_error("unsupported version string '" + vstring + "'");
    }

    H5::H5File handle(path / "contents.h5", H5F_ACC_RDONLY);
    auto ghandle = handle.openGroup(type_name);
    validate_factor_ordered_attribute(ghandle);

    auto num_levels = validate_factor_levels(ghandle.openDataSet("levels"), options.hdf5_buffer_size);
    auto num_codes = validate_factor_codes(ghandle.openDataSet("codes"), num_levels, options.hdf5_buffer_size, true);

    validate_names(ghandle, "names", num_codes, options.hdf5_buffer_size);
}

/**
 * @param path Path to the directory containing the string factor.
 * @param metadata Metadata for the object, typically read from its `OBJECT` file.
 * @param options Validation options.
 * @return Length of the factor.
 */
inline std::size_t height_of_string_factor(const std::filesystem::path& path, [[maybe_unused]] const ObjectMetadata& metadata, [[maybe_unused]] const Options& options) {
    H5::H5File handle(path / "contents.h5", H5F_ACC_RDONLY);
    auto ghandle = handle.openGroup("string_factor");
    auto dhandle = ghandle.openDataSet("codes");
    hsize_t output;
    dhandle.getSpace().getSimpleExtentDims(&output);
    return sanisizer::cast<std::size_t>(output);
}

}

#endif
