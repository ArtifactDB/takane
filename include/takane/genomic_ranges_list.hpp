#ifndef TAKANE_GENOMIC_RANGES_LIST_HPP
#define TAKANE_GENOMIC_RANGES_LIST_HPP

#include "H5Cpp.h"

#include <filesystem>
#include <cstddef>

#include "utils_public.hpp"
#include "utils_compressed_list.hpp"

/**
 * @file genomic_ranges_list.hpp
 * @brief Validation for genomic ranges lists.
 */

namespace takane {

/**
 * @param path Path to the directory containing the genomic ranges list.
 * @param metadata Metadata for the object, typically read from its `OBJECT` file.
 * @param options Validation options.
 */
inline void validate_genomic_ranges_list(const std::filesystem::path& path, const ObjectMetadata& metadata, const Options& options) {
    validate_compressed_list<false>(path, "genomic_ranges_list", "genomic_ranges", metadata, options);
}

/**
 * @param path Path to a directory containing an genomic ranges list.
 * @param metadata Metadata for the object, typically read from its `OBJECT` file.
 * @param options Validation options.
 * @return The length of the list.
 */
inline std::size_t height_of_genomic_ranges_list(const std::filesystem::path& path, const ObjectMetadata& metadata, const Options& options) {
    return height_of_compressed_list(path, "genomic_ranges_list", metadata, options);
}

}

#endif
