#ifndef TAKANE_FASTQ_FILE_HPP
#define TAKANE_FASTQ_FILE_HPP

#include "utils_files.hpp"
#include <filesystem>
#include <stdexcept>
#include <string>

namespace takane {

/**
 * @namespace takane::fastq_file
 * @brief Definitions for FASTQ files.
 */
namespace fastq_file {

/**
 * Application-specific function to check the validity of a FASTQ file and its indices.
 *
 * This should accept a path to the directory containing the FASTQ file, the object metadata, additional reading options, 
 * and a boolean indicating whether or not indices are expected to be present in the directory.
 * It should throw an error if the FASTQ file is not valid, e.g., corrupted file, mismatched indices.
 *
 * If provided, this enables stricter checking of the FASTQ file contents and indices.
 * Currently, we don't look past the magic number to verify the files as this requires a dependency on heavy-duty libraries like, e.g., HTSlib.
 */
inline std::function<void(const std::filesystem::path&, const ObjectMetadata&, const Options&, bool)> strict_check;

/**
 * @param path Path to the directory containing the FASTQ file.
 * @param metadata Metadata for the object, typically read from its `OBJECT` file.
 * @param options Validation options, typically for reading performance.
 */
inline void validate(const std::filesystem::path& path, const ObjectMetadata& metadata, [[maybe_unused]] const Options& options) {
    const auto& fqmap = internal_json::extract_typed_object_from_metadata(metadata.other, "fastq_file");

    const std::string& vstring = internal_json::extract_string_from_typed_object(fqmap, "version", "fastq_file");
    auto version = ritsuko::parse_version_string(vstring.c_str(), vstring.size(), /* skip_patch = */ true);
    if (version.major != 1) {
        throw std::runtime_error("unsupported version string '" + vstring + "'");
    }

    // Checking the quality offset.
    auto oIt = fqmap.find("quality_offset");
    if (oIt != fqmap.end()) {
        const auto& val = oIt->second;
        if (val->type() != millijson::NUMBER) {
            throw std::runtime_error("'fastq_file.quality_offset' property should be a JSON number");
        }

        double offset = reinterpret_cast<const millijson::Number*>(val.get())->value;
        if (offset != 33 && offset != 64) {
            throw std::runtime_error("'fastq_file.quality_offset' property should be either 33 or 64");
        }
    }

    // Check if it's indexed.
    bool indexed = internal_files::is_indexed(fqmap);
    auto fpath = path / "file.fastq.";
    if (indexed) {
        fpath += "bgz";
    } else {
        fpath += "gz";
    }

    auto reader = internal_other::open_reader<byteme::GzipFileReader>(fpath, 10);
    byteme::PerByte<> pb(&reader);
    if (!pb.valid() || pb.get() != '@') {
        throw std::runtime_error("FASTQ file does not start with '@'");
    }

    if (indexed) {
        auto ixpath = fpath;
        ixpath += ".fai";
        if (!std::filesystem::exists(ixpath)) {
            throw std::runtime_error("missing FASTQ index file");
        }

        ixpath = fpath;
        ixpath += ".gzi";
        if (!std::filesystem::exists(ixpath)) {
            throw std::runtime_error("missing BGZF index file");
        }
    }

    if (strict_check) {
        strict_check(path, metadata, options, indexed);
    }
}

}

}

#endif