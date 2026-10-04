#ifndef TAKANE_FASTA_FILE_HPP
#define TAKANE_FASTA_FILE_HPP

#include "utils_files.hpp"

#include "ritsuko/ritsuko.hpp"
#include "byteme/byteme.hpp"

#include <filesystem>
#include <stdexcept>
#include <string>

/**
 * @file fasta_file.hpp
 * @brief Validation for FASTA files.
 */

namespace takane {

/**
 * If `Options::fasta_file_strict_check` is provided, this enables stricter checking of the FASTA file contents and indices.
 * By default, we just look at the first few bytes to verify the files. 
 *
 * @param path Path to the directory containing the FASTA file.
 * @param metadata Metadata for the object, typically read from its `OBJECT` file.
 * @param options Validation options.
 */
inline void validate_fasta_file(const std::filesystem::path& path, const ObjectMetadata& metadata, const Options& options) {
    const std::string type_name = "fasta_file"; // use a separate variable to avoid dangling reference warnings from GCC.

    bool indexed;
    try {
        const auto& famap = extract_json_object(metadata.other, type_name);
        try {
            const std::string& vstring = extract_json_version_string(famap);
            auto version = ritsuko::parse_version_string(vstring.c_str(), vstring.size(), /* skip_patch = */ true);
            if (version.major != 1) {
                throw std::runtime_error("unsupported version string '" + vstring + "'");
            }
        } catch (...) {
            std::throw_with_nested(std::runtime_error("failed to validate 'version'"));
        }

        indexed = is_file_indexed(famap);
        validate_sequence_type(famap);
    } catch (...) {
        std::throw_with_nested(std::runtime_error("failed to validate '" + type_name + "' in the object metadata"));
    }

    // Check if it's indexed.
    auto fpath = path / "file.fasta.";
    if (indexed) {
        fpath += "bgz";
    } else {
        fpath += "gz";
    }
    try {
        check_gzip_file_signature(fpath);
        auto reader = open_reader<byteme::GzipFileReader>(fpath, byteme::GzipFileReaderOptions());
        char first_char;
        if (reader->read(reinterpret_cast<unsigned char*>(&first_char), 1) == 0 || first_char != '>') {
            throw std::runtime_error("FASTA file does not start with '>'");
        }
    } catch (...) {
        std::throw_with_nested(std::runtime_error("failed to validate '" + fpath.filename().string() + "'"));
    }

    if (indexed) {
        auto fixpath = path / "file.fasta.fai";
        if (!std::filesystem::exists(fixpath)) {
            throw std::runtime_error("missing FASTA index file '" + fixpath.filename().string() + "'");
        }

        auto gixpath = fpath;
        gixpath += ".gzi";
        if (!std::filesystem::exists(gixpath)) {
            throw std::runtime_error("missing BGZF index file '" + gixpath.filename().string() + "'");
        }
    }

    if (options.fasta_file_strict_check) {
        options.fasta_file_strict_check(path, metadata, options, indexed);
    }
}

}

#endif
