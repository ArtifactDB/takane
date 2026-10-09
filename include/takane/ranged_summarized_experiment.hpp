#ifndef TAKANE_RANGED_SUMMARIZED_EXPERIMENT_HPP
#define TAKANE_RANGED_SUMMARIZED_EXPERIMENT_HPP

#include "ritsuko/ritsuko.hpp"
#include "sanisizer/sanisizer.hpp"

#include "utils_public.hpp"
#include "utils_json.hpp"
#include "summarized_experiment.hpp"

#include <filesystem>
#include <stdexcept>
#include <string>
#include <exception>

/**
 * @file ranged_summarized_experiment.hpp
 * @brief Validation for ranged summarized experiments.
 */

namespace takane {

/**
 * @cond
 */
void validate(const std::filesystem::path&, const ObjectMetadata&, Options& options);
std::size_t height(const std::filesystem::path&, const ObjectMetadata&, Options& options);
bool derived_from(const std::string&, const std::string&, const Options&);
/**
 * @endcond
 */

/**
 * @param path Path to the directory containing the ranged summarized experiment.
 * @param metadata Metadata for the object, typically read from its `OBJECT` file.
 * @param options Validation options.
 */
inline void validate_ranged_summarized_experiment(const std::filesystem::path& path, const ObjectMetadata& metadata, const Options& options) {
    validate_summarized_experiment(path, metadata, options);

    const std::string type_name = "ranged_summarized_experiment"; // use a separate variable to avoid dangling reference warnings from GCC.
    try {
        const auto& rsemap = extract_json_object(metadata.other, type_name);
        try {
            const std::string& vstring = extract_json_version_string(rsemap);
            auto version = ritsuko::parse_version_string(vstring.c_str(), vstring.size(), /* skip_patch = */ true);
            if (version.major != 1) {
                throw std::runtime_error("unsupported version string '" + vstring + "'");
            }
        } catch (...) {
            std::throw_with_nested(std::runtime_error("failed to read 'version'"));
        }
    } catch (...) {
        std::throw_with_nested(std::runtime_error("failed to read '" + type_name + "' in the object metadata"));
    }

    auto rangedir = path / "row_ranges";
    if (std::filesystem::exists(rangedir)) {
        try {
            auto rangemeta = read_object_metadata(rangedir);
            if (!derived_from(rangemeta.type, "genomic_ranges", options) && !derived_from(rangemeta.type, "genomic_ranges_list", options)) {
                throw std::runtime_error("object should be a 'genomic_ranges', 'genomic_ranges_list', or one of its subclasses");
            }
            validate(rangedir, rangemeta, options);

            auto num_row = height_of_summarized_experiment(path, metadata, options);
            if (height(rangedir, rangemeta, options) != num_row) {
                throw std::runtime_error("object should have height equal to the number of rows of its parent '" + metadata.type + "'");
            }
        } catch (...) {
            std::throw_with_nested(std::runtime_error("failed to validate 'row_ranges'"));
        }
    }
}

}

#endif
