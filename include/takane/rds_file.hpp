#ifndef TAKANE_RDS_FILE_HPP
#define TAKANE_RDS_FILE_HPP

#include "utils_files.hpp"
#include "ritsuko/ritsuko.hpp"

#include <filesystem>
#include <stdexcept>
#include <string>

/**
 * @file rds_file.hpp
 * @brief Validation for RDS files.
 */

namespace takane {

/**
 * If `Options::rds_file_strict_check` is provided, this enables stricter checking of the RDS file contents.
 * By default, we just look at the first few bytes to verify the files. 
 *
 * @param path Path to the directory containing the RDS file.
 * @param metadata Metadata for the object, typically read from its `OBJECT` file.
 * @param options Validation options.
 */
inline void validate_rds_file(const std::filesystem::path& path, const ObjectMetadata& metadata, Options& options) {
    const std::string type_name = "rds_file"; // use a separate variable to avoid dangling reference warnings from GCC.

    try {
        const auto& rdsmap = extract_json_object(metadata.other, type_name);
        try {
            const std::string& vstring = extract_json_version_string(rdsmap);
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

    auto fpath = path / "file.rds";
    try {
        // Check magic numbers.
        check_gzip_file_signature(fpath);
        check_gunzipped_file_signature(fpath, "X\n", 2, "an RDS file");
    } catch (...) {
        std::throw_with_nested(std::runtime_error("failed to validate '" + fpath.filename().string() + "'"));
    }

    if (options.rds_file_strict_check) {
        options.rds_file_strict_check(path, metadata, options);
    }
}

}

#endif
