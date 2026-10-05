#ifndef TAKANE_BIGBED_FILE_HPP
#define TAKANE_BIGBED_FILE_HPP

#include "utils_files.hpp"
#include "utils_json.hpp"

#include "ritsuko/ritsuko.hpp"

#include <filesystem>
#include <stdexcept>
#include <string>
#include <array>

/**
 * @file bigbed_file.hpp
 * @brief Validation for bigBed files.
 */

namespace takane {

/**
 * If `Options::bigbed_file_strict_check` is provided, it is used to perform stricter checking of the bigBed file contents.
 * By default, we don't look past the magic number to verify the files as this requires a dependency on heavy-duty libraries like, e.g., HTSlib.
 *
 * @param path Path to the directory containing the bigBed file.
 * @param metadata Metadata for the object, typically read from its `OBJECT` file.
 * @param options Validation options.
 */
inline void validate_bigbed_file(const std::filesystem::path& path, const ObjectMetadata& metadata, const Options& options) {
    const std::string type_name = "bigbed_file"; // use a separate variable to avoid dangling reference warnings from GCC.

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
    auto ipath = path / "file.bb";
    try {
        std::array<unsigned char, 4> store;
        extract_raw_file_signature(ipath, store.data(), store.size(), /* must_work = */ true);

        std::array<unsigned char, 4> be_magic { 0xEB, 0xF2, 0x89, 0x87 };
        std::array<unsigned char, 4> le_magic { 0x87, 0x89, 0xF2, 0xEB };
        if (store != be_magic && store != le_magic) {
            throw std::runtime_error("incorrect signature for a bigBed file");
        }
    } catch (...) {
        std::throw_with_nested(std::runtime_error("failed to validate '" + ipath.filename().string() + "'"));
    }

    if (options.bigbed_file_strict_check) {
        options.bigbed_file_strict_check(path, metadata, options);
    }
}

}

#endif
