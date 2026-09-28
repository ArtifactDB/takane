#ifndef TAKANE_ATOMIC_VECTOR_LIST_HPP
#define TAKANE_ATOMIC_VECTOR_LIST_HPP

#include <filesystem>
#include <cstddef>

#include "utils_public.hpp"
#include "utils_compressed_list.hpp"

/**
 * @file atomic_vector_list.hpp
 * @brief Validation for atomic vector lists.
 */

namespace takane {

/**
 * @param path Path to the directory containing the atomic vector list.
 * @param metadata Metadata for the object, typically read from its `OBJECT` file.
 * @param options Validation options.
 */
inline void validate_atomic_vector_list(const std::filesystem::path& path, const ObjectMetadata& metadata, const Options& options) {
    validate_compressed_list<false>(path, "atomic_vector_list", "atomic_vector", metadata, options);
}

/**
 * @param path Path to a directory containing an atomic vector list.
 * @param metadata Metadata for the object, typically read from its `OBJECT` file.
 * @param options Validation options.
 * @return The length of the list.
 */
inline std::size_t height_of_atomic_vector_list(const std::filesystem::path& path, const ObjectMetadata& metadata, const Options& options) {
    return height_of_compressed_list(path, "atomic_vector_list", metadata, options);
}

}

#endif
