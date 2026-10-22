#ifndef TAKANE_BUMPY_DATA_FRAME_ARRAY_HPP
#define TAKANE_BUMPY_DATA_FRAME_ARRAY_HPP

#include "H5Cpp.h"

#include <filesystem>
#include <stdexcept>
#include <string>
#include <cstddef>

#include "utils_public.hpp"
#include "utils_bumpy_array.hpp"

/**
 * @file bumpy_data_frame_array.hpp
 * @brief Validation for bumpy data frame matrices.
 */

namespace takane {

/**
 * @param path Path to the directory containing the bumpy data frame array.
 * @param metadata Metadata for the object, typically read from its `OBJECT` file.
 * @param options Validation options.
 */
inline void validate_bumpy_data_frame_array(const std::filesystem::path& path, const ObjectMetadata& metadata, const Options& options) {
    validate_bumpy_array<true>(path, "bumpy_data_frame_array", "DATA_FRAME", metadata, options);
}

/**
 * @param path Path to a directory containing an bumpy data frame array.
 * @param metadata Metadata for the object, typically read from its `OBJECT` file.
 * @param options Validation options.
 * @return The height (i.e., first dimension extent) of the array.
 */
inline std::size_t height_of_bumpy_data_frame_array(const std::filesystem::path& path, const ObjectMetadata& metadata, const Options& options) {
    return height_of_bumpy_array(path, "bumpy_data_frame_array", metadata, options);
}

/**
 * @param path Path to a directory containing an bumpy data frame array.
 * @param metadata Metadata for the object, typically read from its `OBJECT` file.
 * @param options Validation options.
 * @return Vector containing the dimensions of the array.
 */
inline std::vector<std::size_t> dimensions_of_bumpy_data_frame_array(const std::filesystem::path& path, const ObjectMetadata& metadata, const Options& options) {
    return dimensions_of_bumpy_array(path, "bumpy_data_frame_array", metadata, options);
}

}

#endif
