#ifndef TAKANE_BUMPY_ATOMIC_ARRAY_HPP
#define TAKANE_BUMPY_ATOMIC_ARRAY_HPP

#include "H5Cpp.h"

#include <filesystem>
#include <stdexcept>
#include <string>

#include "utils_public.hpp"
#include "utils_bumpy_array.hpp"

/**
 * @file bumpy_atomic_array.hpp
 * @brief Validation for bumpy atomic matrices.
 */

namespace takane {

/**
 * @param path Path to the directory containing the bumpy atomic array.
 * @param metadata Metadata for the object, typically read from its `OBJECT` file.
 * @param options Validation options.
 */
inline void validate_bumpy_atomic_array(const std::filesystem::path& path, const ObjectMetadata& metadata, const Options& options) {
    validate_bumpy_array<false>(path, "bumpy_atomic_array", "atomic_vector", metadata, options);
}

/**
 * @param path Path to a directory containing an bumpy atomic array.
 * @param metadata Metadata for the object, typically read from its `OBJECT` file.
 * @param options Validation options.
 * @return The height (i.e., first dimension extent) of the array.
 */
inline std::size_t height_of_bumpy_atomic_array(const std::filesystem::path& path, const ObjectMetadata& metadata, const Options& options) {
    return height_of_bumpy_array(path, "bumpy_atomic_array", metadata, options);
}

/**
 * @param path Path to a directory containing an bumpy atomic array.
 * @param metadata Metadata for the object, typically read from its `OBJECT` file.
 * @param options Validation options.
 * @return Vector containing the dimensions of the array.
 */
inline std::vector<std::size_t> dimensions_of_bumpy_atomic_array(const std::filesystem::path& path, const ObjectMetadata& metadata, const Options& options) {
    return dimensions_of_bumpy_array(path, "bumpy_atomic_array", metadata, options);
}

}

#endif
