#ifndef TAKANE_BAM_FILE_HPP
#define TAKANE_BAM_FILE_HPP

#include "utils_files.hpp"
#include "utils_json.hpp"

#include "ritsuko/ritsuko.hpp"

#include <filesystem>
#include <stdexcept>
#include <string>

/**
 * @file bam_file.hpp
 * @brief Validation for BAM files.
 */

namespace takane {

/**
 * If `Options::bam_file_strict_check` is provided, it is used to perform stricter checking of the BAM file contents and indices.
 * By default, we don't look past the magic number to verify the files as this requires a dependency on heavy-duty libraries like, e.g., HTSlib.
 *
 * @param path Path to the directory containing the BAM file.
 * @param metadata Metadata for the object, typically read from its `OBJECT` file.
 * @param options Validation options.
 */
inline void validate_bam_file(const std::filesystem::path& path, const ObjectMetadata& metadata, const Options& options) {
    const std::string type_name = "bam_file"; // use a separate variable to avoid dangling reference warnings from GCC.

    try {
        const auto& type_meta = extract_json_object(metadata.other, type_name);
        try {
            const std::string& vstring = extract_json_version_string(type_meta);
            auto version = ritsuko::parse_version_string(vstring.c_str(), vstring.size(), /* skip_patch = */ true);
            if (version.major != 1) {
                throw std::runtime_error("unsupported version string '" + vstring + "'");
            }
        } catch (...) {
            std::throw_with_nested(std::runtime_error("failed to validate 'version'"));
        }
    } catch (...) {
        std::throw_with_nested(std::runtime_error("failed to validate '" + type_name + "' in the object metadata"));
    }

    // Magic numbers taken from https://samtools.github.io/hts-specs/SAMv1.pdf
    auto ipath = path / "file.bam";
    try {
        check_gzip_file_signature(ipath);
        check_gunzipped_file_signature(ipath, "BAM\1", 4, "a BAM file");
    } catch (...) {
        std::throw_with_nested(std::runtime_error("failed to validate '" + ipath.filename().string() + "'"));
    }

    auto baixpath = ipath;
    baixpath += ".bai";
    if (std::filesystem::exists(baixpath)) {
        try {
            check_raw_file_signature(baixpath, "BAI\1", 4, "a BAM index");
        } catch (...) {
            std::throw_with_nested(std::runtime_error("failed to validate '" + baixpath.filename().string() + "'"));
        }
    }

    // Magic number taken from https://samtools.github.io/hts-specs/CSIv1.pdf
    auto csixpath = ipath;
    csixpath += ".csi";
    if (std::filesystem::exists(csixpath)) {
        try {
            check_gzip_file_signature(csixpath);
            check_gunzipped_file_signature(csixpath, "CSI\1", 4, "a CSI index");
        } catch (...) {
            std::throw_with_nested(std::runtime_error("failed to validate '" + csixpath.filename().string() + "'"));
        }
    }

    if (options.bam_file_strict_check) {
        options.bam_file_strict_check(path, metadata, options);
    }
}

}

#endif
