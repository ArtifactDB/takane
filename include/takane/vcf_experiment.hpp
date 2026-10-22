#ifndef TAKANE_VCF_EXPERIMENT_HPP
#define TAKANE_VCF_EXPERIMENT_HPP

#include <string>
#include <stdexcept>
#include <filesystem>
#include <exception>
#include <cstddef>

#include "ritsuko/ritsuko.hpp"
#include "byteme/byteme.hpp"

#include "utils_public.hpp"
#include "utils_string.hpp"
#include "utils_summarized_experiment.hpp"
#include "utils_json.hpp"
#include "utils_files.hpp"

/**
 * @file vcf_experiment.hpp
 * @brief Validation for VCF-based experiments.
 */

namespace takane {

/**
 * @cond
 */
// Format specification taken from https://samtools.github.io/hts-specs/VCFv4.1.pdf.
inline std::pair<std::size_t, std::size_t> scan_vcf_dimensions(const std::filesystem::path& path, bool expanded, bool parallel) {
    check_gzip_file_signature(path);
    auto reader = open_reader<byteme::GzipFileReader>(path, byteme::GzipFileReaderOptions());
    auto pb = wrap_reader_for_bytes<char>(std::move(reader), parallel);

    // Checking the signature.
    {
        const std::string expected = "##fileformat=VCFv";
        const auto len = expected.size();
        bool okay = pb->valid();

        for (I<decltype(len)> i = 0; i < len; ++i) {
            if (!okay) {
                throw std::runtime_error("not enough bytes to extract a signature for a VCF file");
            }
            if (pb->get() != expected[i]) {
                throw std::runtime_error("incorrect signature for a VCF file");
            }
            okay = pb->advance();
        }
    }

    // Scanning until we find a line that doesn't start with '##'.
    while (true) {
        if (pb->get() == '\n') {
            bool escape = false;
            for (int i = 0; i < 2; ++i) {
                if (!pb->advance()) {
                    throw std::runtime_error("premature end to the VCF file");
                }
                if (pb->get() != '#') {
                    escape = true;
                    break;
                }
            }
            if (escape) {
                break;
            }
        }
        if (!pb->advance()) {
            throw std::runtime_error("premature end to the VCF file");
        }
    }

    // Scanning the header line to count the number of samples.
    std::size_t num_samples = 0;
    {
        std::size_t num_indents = 0;
        while (true) {
            char current = pb->get();
            if (current == '\t') {
                sanisizer::increment(num_indents);
            } else if (current == '\n') {
                pb->advance(); // skip past the newline.
                break;
            }
            if (!pb->advance()) {
                throw std::runtime_error("premature end to the VCF file");
            }
        }

        if (num_indents < 8) {
            throw std::runtime_error("expected at least 9 fields in the VCF header line, including 'FORMAT'");
        }
        num_samples = num_indents - 8;
    }

    std::size_t expected_rows = 0;
    if (expanded) {
        while (pb->valid()) {
            sanisizer::increment(expected_rows);

            // Scanning up to the ALT field, which is the 5th one. We need this to
            // find the number of alternative alleles (and thus the expansion of rows).
            std::size_t num_indents = 0;
            while (true) {
                char current = pb->get();
                if (current == '\t') {
                    sanisizer::increment(num_indents);
                    if (num_indents == 4) { 
                        if (!pb->advance()) { // get past this indent.
                            throw std::runtime_error("premature end of line for VCF record");
                        }
                        break;
                    }
                } else if (current == '\n') {
                    throw std::runtime_error("premature end of line for VCF record");
                }
                if (!pb->advance()) {
                    throw std::runtime_error("premature end of line for VCF record");
                }
            }

            // Checking that we don't have any commas if it's expanded.
            while (true) {
                char current = pb->get();
                if (current == ',') {
                    throw std::runtime_error("expected a 1:1 mapping of rows to alternative alleles for an expanded VCF");
                } else if (current == '\t') {
                    break;
                } else if (current == '\n') {
                    throw std::runtime_error("premature end of line for VCF record");
                }
                if (!pb->advance()) {
                    throw std::runtime_error("premature end of line for VCF record");
                }
            }

            // Chomping the rest of the line; we assume all lines are newline-terminated.
            while (true) {
                if (pb->get() == '\n') {
                    pb->advance(); // skip past the newline.
                    break;
                } else {
                    if (!pb->advance()) {
                        throw std::runtime_error("premature end of line for VCF record");
                    }
                }
            }
        }

    } else {
        if (pb->valid()) {
            while (true) {
                if (pb->get() == '\n') {
                    sanisizer::increment(expected_rows); // assume all files are newline-terminated.
                    if (!pb->advance()) {
                        break;
                    }
                } else {
                    if (!pb->advance()) {
                        throw std::runtime_error("premature end of line for VCF record");
                    }
                }
            }
        }
    }

    return std::make_pair(expected_rows, num_samples);
}
/**
 * @endcond
 */

/**
 * @param path Path to the directory containing the VCF experiment.
 * @param metadata Metadata for the object, typically read from its `OBJECT` file.
 * @param options Validation options.
 */
inline void validate_vcf_experiment(const std::filesystem::path& path, const ObjectMetadata& metadata, const Options& options) {
    const std::string type_name = "vcf_experiment"; // use a separate variable to avoid dangling reference warnings from GCC.

    std::pair<std::size_t, std::size_t> dims;
    bool exp = false;
    try {
        const auto& vcfmap = extract_json_object(metadata.other, type_name);
        try {
            const std::string& vstring = extract_json_version_string(vcfmap);
            auto version = ritsuko::parse_version_string(vstring.c_str(), vstring.size(), /* skip_patch = */ true);
            if (version.major != 1) {
                throw std::runtime_error("unsupported version string '" + vstring + "'");
            }
        } catch (...) {
            std::throw_with_nested(std::runtime_error("failed to read 'version'"));
        }

        dims = extract_summarized_experiment_dimensions(vcfmap);

        auto eIt = vcfmap.find("expanded");
        if (eIt == vcfmap.end()) {
            throw std::runtime_error("expected a 'expanded' property");
        }
        const auto& val = eIt->second;
        if (val->type() != millijson::BOOLEAN) {
            throw std::runtime_error("'expanded' property should be a JSON boolean");
        }
        exp = reinterpret_cast<const millijson::Boolean*>(val.get())->value();
    } catch (...) {
        std::throw_with_nested(std::runtime_error("failed to validate '" + type_name + "' in the object metadata"));
    }

    auto ipath = path / "file.vcf.gz";
    std::pair<std::size_t, std::size_t> obs_dims;
    try {
        obs_dims = scan_vcf_dimensions(ipath, exp, options.parallel_reads);
    } catch (...) {
        std::throw_with_nested(std::runtime_error("failed to parse '" + ipath.filename().string() + "'"));
    }

    if (obs_dims.first != dims.first) {
        throw std::runtime_error("first dimension in object metadata does not match the number of records in '" + ipath.filename().string() + "'");
    }
    if (obs_dims.second != dims.second) {
        throw std::runtime_error("second dimension in object metadata does not match the number of samples in '" + ipath.filename().string() + "'");
    }
}

/**
 * @param path Path to a directory containing a VCF experiment.
 * @param metadata Metadata for the object, typically read from its `OBJECT` file.
 * @param options Validation options.
 * @return Number of rows in the VCF experiment.
 */
inline std::size_t height_of_vcf_experiment([[maybe_unused]] const std::filesystem::path& path, const ObjectMetadata& metadata, [[maybe_unused]] const Options& options) {
    const std::string type_name = "vcf_experiment"; // use a separate variable to avoid dangling reference warnings from GCC.
    const auto& vcfmap = extract_json_object(metadata.other, type_name);
    auto dims = extract_summarized_experiment_dimensions(vcfmap);
    return dims.first;
}

/**
 * @param path Path to a directory containing a VCF experiment.
 * @param metadata Metadata for the object, typically read from its `OBJECT` file.
 * @param options Validation options.
 * @return A vector of length 2 containing the dimensions of the VCF experiment.
 */
inline std::vector<std::size_t> dimensions_of_vcf_experiment([[maybe_unused]] const std::filesystem::path& path, const ObjectMetadata& metadata, [[maybe_unused]] const Options& options) {
    const std::string type_name = "vcf_experiment"; // use a separate variable to avoid dangling reference warnings from GCC.
    const auto& vcfmap = extract_json_object(metadata.other, type_name);
    auto dims = extract_summarized_experiment_dimensions(vcfmap);
    return std::vector<std::size_t>{ dims.first, dims.second };
}

}

#endif
