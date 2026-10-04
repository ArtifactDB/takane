#ifndef TAKANE_BED_FILE_HPP
#define TAKANE_BED_FILE_HPP

#include "utils_files.hpp"

#include "ritsuko/ritsuko.hpp"

#include <filesystem>
#include <stdexcept>
#include <string>

/**
 * @file bed_file.hpp
 * @brief Validation for BED files.
 */

namespace takane {

/**
 * If `Options::bed_file_strict_check` is provided, it is used to perform stricter checking of the BED file contents and indices.
 * Currently, we don't look past the magic number to verify the files as this requires a dependency on heavy-duty libraries like, e.g., HTSlib.
 *
 * @param path Path to the directory containing the BED file.
 * @param metadata Metadata for the object, typically read from its `OBJECT` file.
 * @param options Validation options.
 */
inline void validate_bed_file(const std::filesystem::path& path, const ObjectMetadata& metadata, const Options& options) {
    const std::string type_name = "bed_file"; // use a separate variable to avoid dangling reference warnings from GCC.

    bool indexed;
    try {
        const auto& bedmap = extract_json_object(metadata.other, type_name);
        try {
            const std::string& vstring = extract_json_version_string(bedmap);
            auto version = ritsuko::parse_version_string(vstring.c_str(), vstring.size(), /* skip_patch = */ true);
            if (version.major != 1) {
                throw std::runtime_error("unsupported version string '" + vstring + "'");
            }
        } catch (...) {
            std::throw_with_nested(std::runtime_error("failed to validate 'version'"));
        }
        indexed = is_file_indexed(bedmap);
    } catch (...) {
        std::throw_with_nested(std::runtime_error("failed to validate '" + type_name + "' in the object metadata"));
    }

    // Check if it's indexed.
    auto fpath = path / "file.bed.";
    if (indexed) {
        fpath += "bgz";
    } else {
        fpath += "gz";
    }
    try {
        check_gzip_file_signature(fpath);
    } catch (...) {
        std::throw_with_nested(std::runtime_error("failed to validate '" + fpath.filename().string() + "'"));
    }

    if (indexed) {
        auto ixpath = fpath;
        ixpath += ".tbi";
        try {
            check_gzip_file_signature(ixpath);
            check_gunzipped_file_signature(ixpath, "TBI\1", 4, "a tabix file");
        } catch (...) {
            std::throw_with_nested(std::runtime_error("failed to validate '" + fpath.filename().string() + "'"));
        }
    }

    if (options.bed_file_strict_check) {
        options.bed_file_strict_check(path, metadata, options, indexed);
    }
}

}

#endif
