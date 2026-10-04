#ifndef TAKANE_FASTQ_FILE_HPP
#define TAKANE_FASTQ_FILE_HPP

#include "utils_files.hpp"
#include "ritsuko/ritsuko.hpp"
#include "byteme/byteme.hpp"

#include <filesystem>
#include <stdexcept>
#include <string>

/**
 * @file fastq_file.hpp
 * @brief Validation for FASTQ files.
 */

namespace takane {

/**
 * If `Options::fastq_file_strict_check` is provided, this enables stricter checking of the FASTQ file contents and indices.
 * By default, we just look at the first few bytes to verify the files.
 *
 * @param path Path to the directory containing the FASTQ file.
 * @param metadata Metadata for the object, typically read from its `OBJECT` file.
 * @param options Validation options.
 */
inline void validate_fastq_file(const std::filesystem::path& path, const ObjectMetadata& metadata, const Options& options) {
    const std::string type_name = "fastq_file"; // use a separate variable to avoid dangling reference warnings from GCC.

    bool indexed;
    try {
        const auto& fqmap = extract_json_object(metadata.other, type_name);
        try {
            const std::string& vstring = extract_json_version_string(fqmap);
            auto version = ritsuko::parse_version_string(vstring.c_str(), vstring.size(), /* skip_patch = */ true);
            if (version.major != 1) {
                throw std::runtime_error("unsupported version string '" + vstring + "'");
            }
        } catch (...) {
            std::throw_with_nested(std::runtime_error("failed to validate 'version'"));
        }

        validate_sequence_type(fqmap);
        indexed = is_file_indexed(fqmap);

        // Checking the quality type and offset.
        enum Failure { TYPE, OFFSET };
        Failure who_failed = TYPE;
        try {
            const std::string qtype_name = "quality_type"; // again, avoid dangling reference warnings.
            const std::string& qtype = extract_json_string(fqmap, qtype_name);

            if (qtype == "phred") {
                who_failed = OFFSET;
                auto oIt = fqmap.find("quality_offset");
                if (oIt == fqmap.end()) {
                    throw std::runtime_error("property is not present");
                }

                const auto& val = oIt->second;
                if (val->type() != millijson::NUMBER) {
                    throw std::runtime_error("property should be a JSON number");
                }

                double offset = reinterpret_cast<const millijson::Number*>(val.get())->value();
                if (offset != 33 && offset != 64) {
                    throw std::runtime_error("property should be either 33 or 64");
                }
            } else if (qtype != "solexa") {
                throw std::runtime_error("unknown value '" + qtype + "'");
            }

        } catch (...) {
            std::string desc;
            switch (who_failed) {
                case TYPE: desc = "quality_type"; break;
                case OFFSET: desc = "quality_offset"; break;
            }
            std::throw_with_nested(std::runtime_error("failed to validate '" + desc + "'"));
        }
    } catch (...) {
        std::throw_with_nested(std::runtime_error("failed to validate '" + type_name + "' in the object metadata"));
    }

    // Check if it's indexed.
    auto fpath = path / "file.fastq.";
    if (indexed) {
        fpath += "bgz";
    } else {
        fpath += "gz";
    }
    try {
        check_gzip_file_signature(fpath);
        auto reader = open_reader<byteme::GzipFileReader>(fpath, byteme::GzipFileReaderOptions());
        char first_val;
        if (reader->read(reinterpret_cast<unsigned char*>(&first_val), 1) == 0 || first_val != '@') {
            throw std::runtime_error("FASTQ file does not start with '@'");
        }
    } catch (...) {
        std::throw_with_nested(std::runtime_error("failed to validate '" + fpath.filename().string() + "'"));
    }

    if (indexed) {
        auto fixpath = path / "file.fastq.fai";
        if (!std::filesystem::exists(fixpath)) {
            throw std::runtime_error("missing FASTQ index file '" + fixpath.filename().string() + "'");
        }

        auto gixpath = fpath;
        gixpath += ".gzi";
        if (!std::filesystem::exists(gixpath)) {
            throw std::runtime_error("missing BGZF index file '" + gixpath.filename().string() + "'");
        }
    }

    if (options.fastq_file_strict_check) {
        options.fastq_file_strict_check(path, metadata, options, indexed);
    }
}

}

#endif
