#ifndef TAKANE_GMT_FILE_HPP
#define TAKANE_GMT_FILE_HPP

#include "utils_files.hpp"
#include "ritsuko/ritsuko.hpp"

#include <filesystem>
#include <stdexcept>
#include <string>

/**
 * @file gmt_file.hpp
 * @brief Validation for GMT files.
 */

namespace takane {

/**
 * If `Options::gmt_file_strict_check` is provided, this enables stricter checking of the GMT file contents.
 * By default, we just look at the first few bytes to verify the files.
 *
 * @param path Path to the directory containing the GMT file.
 * @param metadata Metadata for the object, typically read from its `OBJECT` file.
 * @param options Validation options.
 */
inline void validate_gmt_file(const std::filesystem::path& path, const ObjectMetadata& metadata, const Options& options) {
    const std::string type_name = "gmt_file"; // use a separate variable to avoid dangling reference warnings from GCC.

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

    auto fpath = path / "file.gmt.gz";
    try {
        check_gzip_file_signature(fpath);
    } catch (...) {
        std::throw_with_nested(std::runtime_error("failed to validate '" + fpath.filename().string() + "'"));
    }

    if (options.gmt_file_strict_check) {
        options.gmt_file_strict_check(path, metadata, options);
    }
}

}

#endif
