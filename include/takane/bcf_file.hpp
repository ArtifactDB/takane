#ifndef TAKANE_BCF_FILE_HPP
#define TAKANE_BCF_FILE_HPP

#include "utils_files.hpp"
#include <filesystem>
#include <stdexcept>
#include <string>

namespace takane {

/**
 * @namespace takane::bcf_file
 * @brief Definitions for BCF files.
 */
namespace bcf_file {

/**
 * Application-specific function to check the validity of a BCF file and its indices.
 *
 * This should accept a path to the directory containing the BCF file and indices, the object metadata, and additional reading options.
 * It should throw an error if the BCF file is not valid, e.g., corrupted file, mismatched indices.
 *
 * If provided, this enables stricter checking of the BCF file contents and indices.
 * Currently, we don't look past the magic number to verify the files as this requires a dependency on heavy-duty libraries like, e.g., HTSlib.
 */
inline std::function<void(const std::filesystem::path&, const ObjectMetadata&, const Options&)> strict_check;

/**
 * @param path Path to the directory containing the BCF file.
 * @param metadata Metadata for the object, typically read from its `OBJECT` file.
 * @param options Validation options, typically for reading performance.
 */
inline void validate(const std::filesystem::path& path, const ObjectMetadata& metadata, [[maybe_unused]] const Options& options) {
    const std::string& vstring = internal_json::extract_version_for_type(metadata.other, "bcf_file");
    auto version = ritsuko::parse_version_string(vstring.c_str(), vstring.size(), /* skip_patch = */ true);
    if (version.major != 1) {
        throw std::runtime_error("unsupported version string '" + vstring + "'");
    }

    // Magic number taken from https://samtools.github.io/hts-specs/BCFv2_qref.pdf
    auto ipath = path / "file.bcf";
    internal_files::check_signature(ipath, "BCF\2\1", 5, "BCF");

    // Magic number taken from https://samtools.github.io/hts-specs/CSIv1.pdf
    auto ixpath = ipath;
    ixpath += ".csi";
    if (std::filesystem::exists(ixpath)) {
        internal_files::check_signature(ixpath, "CSI\1", 4, "CSI index");
    }

    if (strict_check) {
        strict_check(path, metadata, options);
    }
}

}

}

#endif
