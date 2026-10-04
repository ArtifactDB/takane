#ifndef TAKANE_BIGWIG_FILE_HPP
#define TAKANE_BIGWIG_FILE_HPP

#include "utils_files.hpp"

#include "ritsuko/ritsuko.hpp"

#include <filesystem>
#include <stdexcept>
#include <string>

/**
 * @file bigwig_file.hpp
 * @brief Validation for bigWig files.
 */

namespace takane {

/**
 * If `Options::bigwig_file_strict_check` is provided, it is used to perform stricter checking of the bigWig file contents.
 * By default, we don't look past the magic number to verify the files as this requires a dependency on heavy-duty libraries like, e.g., HTSlib.
 *
 * @param path Path to the directory containing the bigWig file.
 * @param metadata Metadata for the object, typically read from its `OBJECT` file.
 * @param options Validation options.
 */
inline void validate_bigwig_file(const std::filesystem::path& path, const ObjectMetadata& metadata, const Options& options) {
    const std::string type_name = "bigwig_file"; // use a separate variable to avoid dangling reference warnings from GCC.

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

    // Magic numbers taken from https://data.broadinstitute.org/igv/projects/downloads/2.1/IGVDistribution_2.1.11/src/org/broad/igv/bbfile/BBFileHeader.java,
    // which seems to use the magic number to detect endianness of the file as well.
    auto ipath = path / "file.bw";
    try {
        std::array<unsigned char, 4> store;
        extract_file_signature(ipath, store.data(), store.size());

        std::array<unsigned char, 4> be_magic { 0x26, 0xFC, 0x8F, 0x88 };
        std::array<unsigned char, 4> le_magic { 0x88, 0x8F, 0xFC, 0x26 };
        if (store != be_magic && store != le_magic) {
            throw std::runtime_error("incorrect bigWig file signature");
        }
    } catch (...) {
        std::throw_with_nested(std::runtime_error("failed to validate '" + path.filename().string() + "'"));
    }

    if (options.bigwig_file_strict_check) {
        options.bigwig_file_strict_check(path, metadata, options);
    }
}

}

#endif
