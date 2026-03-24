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
 * @namespace takane::rds_file
 * @brief Definitions for RDS files.
 */
namespace rds_file {

/**
 * If `Options::rds_file_strict_check` is provided, this enables stricter checking of the RDS file contents.
 * By default, we just look at the first few bytes to verify the files. 
 *
 * @param path Path to the directory containing the RDS file.
 * @param metadata Metadata for the object, typically read from its `OBJECT` file.
 * @param options Validation options.
 */
inline void validate(const std::filesystem::path& path, const ObjectMetadata& metadata, Options& options) {
    const std::string type_name = "rds_file"; // use a separate variable to avoid dangling reference warnings from GCC.
    const auto& rdsmap = internal_json::extract_typed_object_from_metadata(metadata.other, type_name);

    const std::string version_name = "version"; // again, avoid dangling reference warnings.
    const std::string& vstring = internal_json::extract_string_from_typed_object(rdsmap, version_name, type_name);
    auto version = ritsuko::parse_version_string(vstring.c_str(), vstring.size(), /* skip_patch = */ true);
    if (version.major != 1) {
        throw std::runtime_error("unsupported version string '" + vstring + "'");
    }

    auto fpath = path / "file.rds";

    // Check magic numbers.
    internal_files::check_gzip_signature(fpath);
    internal_files::check_gunzipped_signature(fpath, "X\n", 2, "RDS");

    if (options.rds_file_strict_check) {
        options.rds_file_strict_check(path, metadata, options);
    }
}

}

}

#endif
