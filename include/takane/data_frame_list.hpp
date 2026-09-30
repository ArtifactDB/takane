#ifndef TAKANE_DATA_FRAME_LIST_HPP
#define TAKANE_DATA_FRAME_LIST_HPP

#include <filesystem>
#include <cstddef>

#include "utils_public.hpp"
#include "utils_compressed_list.hpp"

/**
 * @file data_frame_list.hpp
 * @brief Validation for data frame lists.
 */

namespace takane {

/**
 * @param path Path to the directory containing the data frame list.
 * @param metadata Metadata for the object, typically read from its `OBJECT` file.
 * @param options Validation options.
 */
inline void validate_data_frame_list(const std::filesystem::path& path, const ObjectMetadata& metadata, const Options& options) {
    validate_compressed_list<true>(path, "data_frame_list", "DATA_FRAME", metadata, options);
}

/**
 * @param path Path to a directory containing an data frame list.
 * @param metadata Metadata for the object, typically read from its `OBJECT` file.
 * @param options Validation options.
 * @return The length of the list.
 */
inline std::size_t height_of_data_frame_list(const std::filesystem::path& path, const ObjectMetadata& metadata, const Options& options) {
    return height_of_compressed_list(path, "data_frame_list", metadata, options);
}

}

#endif
