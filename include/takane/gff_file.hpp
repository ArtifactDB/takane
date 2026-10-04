#ifndef TAKANE_GFF_FILE_HPP
#define TAKANE_GFF_FILE_HPP

#include "utils_files.hpp"

#include "ritsuko/ritsuko.hpp"

#include <filesystem>
#include <stdexcept>
#include <string>

/**
 * @file gff_file.hpp
 * @brief Validation for GFF files.
 */

namespace takane {

/**
 * If `Options::gff_file_strict_check` is provided, this enables stricter checking of the GFF file contents.
 * By default, we just look at the first few bytes to verify the files. 
 *
 * @param path Path to the directory containing the GFF file.
 * @param metadata Metadata for the object, typically read from its `OBJECT` file.
 * @param options Validation options.
 */
inline void validate_gff_file(const std::filesystem::path& path, const ObjectMetadata& metadata, const Options& options) {
    const std::string type_name = "gff_file"; // use a separate variable to avoid dangling reference warnings from GCC.

    auto fpath = path / "file.";
    bool indexed, is_gff3 = false;
    try {
        const auto& gffmap = extract_json_object(metadata.other, type_name);
        try {
            const std::string& vstring = extract_json_version_string(gffmap);
            auto version = ritsuko::parse_version_string(vstring.c_str(), vstring.size(), /* skip_patch = */ true);
            if (version.major != 1) {
                throw std::runtime_error("unsupported version string '" + vstring + "'");
            }
        } catch (...) {
            std::throw_with_nested(std::runtime_error("failed to validate 'version'"));
        }

        try {
            const std::string format_name = "format"; // again, avoid dangling reference warnings.
            const std::string& fstring = extract_json_string(gffmap, format_name);
            if (fstring == "GFF2") {
                fpath += "gff2";
            } else if (fstring == "GFF3") {
                is_gff3 = true;
                fpath += "gff3";
            } else {
                throw std::runtime_error("unknown value '" + fstring + "'");
            }
        } catch (...) {
            std::throw_with_nested(std::runtime_error("failed to validate 'format'"));
        }

        indexed = is_file_indexed(gffmap);
    } catch (...) {
        std::throw_with_nested(std::runtime_error("failed to validate '" + type_name + "' in the object metadata"));
    }

    fpath += ".";
    if (indexed) {
        fpath += "bgz";
    } else {
        fpath += "gz";
    }
    try {
        check_gzip_file_signature(fpath);
        if (is_gff3) {
            const std::string signature = "##gff-version 3";
            check_gunzipped_file_signature(fpath, signature.c_str(), signature.size(), "a GFF3 file");
        }
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
            std::throw_with_nested(std::runtime_error("failed to validate '" + ixpath.filename().string() + "'"));
        }
    }

    if (options.gff_file_strict_check) {
        options.gff_file_strict_check(path, metadata, options, indexed);
    }
}

}

#endif
