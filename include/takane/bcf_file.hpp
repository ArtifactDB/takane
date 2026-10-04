#ifndef TAKANE_BCF_FILE_HPP
#define TAKANE_BCF_FILE_HPP

#include "utils_files.hpp"
#include "utils_json.hpp"

#include "ritsuko/ritsuko.hpp"

#include <filesystem>
#include <stdexcept>
#include <string>

/**
 * @file bcf_file.hpp
 * @brief Validation for BCF files.
 */

namespace takane {

/**
 * If `Options::bcf_file_strict_check` is provided, it is used to perform stricter checking of the BCF file contents and indices.
 * By default, we don't look past the magic number to verify the files as this requires a dependency on heavy-duty libraries like, e.g., HTSlib.
 *
 * @param path Path to the directory containing the BCF file.
 * @param metadata Metadata for the object, typically read from its `OBJECT` file.
 * @param options Validation options.
 */
inline void validate_bcf_file(const std::filesystem::path& path, const ObjectMetadata& metadata, const Options& options) {
    const std::string type_name = "bcf_file"; // use a separate variable to avoid dangling reference warnings from GCC.

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

    // Magic number taken from https://samtools.github.io/hts-specs/BCFv2_qref.pdf
    // We relax it a little to support both BCF1 and BCF2+ formats.
    auto ipath = path / "file.bcf";
    try {
        check_gzip_file_signature(ipath);
        check_gunzipped_file_signature(ipath, "BCF", 3, "a BCF file");
    } catch (...) {
        std::throw_with_nested(std::runtime_error("failed to validate '" + ipath.filename().string() + "'"));
    }

    // Magic number taken from https://samtools.github.io/hts-specs/tabix.pdf
    auto tbixpath = ipath;
    tbixpath += ".tbi";
    if (std::filesystem::exists(tbixpath)) {
        try {
            check_gzip_file_signature(tbixpath);
            check_gunzipped_file_signature(tbixpath, "TBI\1", 4, "a tabix file");
        } catch (...) {
            std::throw_with_nested(std::runtime_error("failed to validate '" + tbixpath.filename().string() + "'"));
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

    if (options.bcf_file_strict_check) {
        options.bcf_file_strict_check(path, metadata, options);
    }
}

}

#endif
