#ifndef TAKANE_SUMMARIZED_EXPERIMENT_HPP
#define TAKANE_SUMMARIZED_EXPERIMENT_HPP

#include "ritsuko/ritsuko.hpp"
#include "sanisizer/sanisizer.hpp"

#include "utils_public.hpp"
#include "utils_other.hpp"
#include "utils_summarized_experiment.hpp"

#include <filesystem>
#include <stdexcept>
#include <exception>
#include <string>
#include <cstddef>

/**
 * @file summarized_experiment.hpp
 * @brief Validation for summarized experiments.
 */

namespace takane {

/**
 * @cond
 */
void validate(const std::filesystem::path&, const ObjectMetadata&, const Options& options);
size_t height(const std::filesystem::path&, const ObjectMetadata&, const Options& options);
std::vector<std::size_t> dimensions(const std::filesystem::path&, const ObjectMetadata&, const Options& options);
bool satisfies_interface(const std::string&, const std::string&, const Options&);
/**
 * @endcond
 */

/**
 * @param path Path to the directory containing the summarized experiment.
 * @param metadata Metadata for the object, typically read from its `OBJECT` file.
 * @param options Validation options.
 */
inline void validate_summarized_experiment(const std::filesystem::path& path, const ObjectMetadata& metadata, const Options& options) {
    const std::string type_name = "summarized_experiment"; // use a separate variable to avoid dangling reference warnings from GCC.

    std::size_t num_rows, num_cols;
    try {
        const auto& semap = extract_json_object(metadata.other, type_name);
        try {
            const std::string& vstring = extract_json_version_string(semap);
            auto version = ritsuko::parse_version_string(vstring.c_str(), vstring.size(), /* skip_patch = */ true);
            if (version.major != 1) {
                throw std::runtime_error("unsupported version string '" + vstring + "'");
            }
        } catch (...) {
            std::throw_with_nested(std::runtime_error("failed to validate 'version'"));
        }

        try {
            auto dims = extract_summarized_experiment_dimensions(semap);
            num_rows = dims.first;
            num_cols = dims.second;
        } catch (...) {
            std::throw_with_nested(std::runtime_error("failed to read 'dimensions'"));
        }
    } catch (...) {
        std::throw_with_nested(std::runtime_error("failed to validate '" + type_name + "' in the object metadata"));
    }

    // Checking the assays. The directory is also allowed to not exist, in which case we have no assays.
    auto adir = path / "assays";
    if (std::filesystem::exists(adir)) {
        try {
            auto num_assays = [&]{
                try {
                    return extract_summarized_experiment_names(adir / "names.json").size();
                } catch (...) {
                    std::throw_with_nested(std::runtime_error("failed to read 'names.json'"));
                }
            }();

            for (I<decltype(num_assays)> i = 0; i < num_assays; ++i) {
                auto aname = std::to_string(i);
                auto apath = adir / aname;
                try {
                    auto ameta = read_object_metadata(apath);
                    validate(apath, ameta, options);

                    auto dims = dimensions(apath, ameta, options);
                    if (dims.size() < 2) {
                        throw std::runtime_error("object should have two or more dimensions");
                    }
                    if (dims[0] != num_rows) {
                        throw std::runtime_error("object should have the same number of rows as its parent '" + metadata.type + "'");
                    }
                    if (dims[1] != num_cols) {
                        throw std::runtime_error("object should have the same number of columns as its parent '" + metadata.type + "'");
                    }
                } catch (...) {
                    std::throw_with_nested(std::runtime_error("failed to validate assay " + aname));
                }
            }

            const auto num_dir_obj = count_directory_entries(adir);
            if (!sanisizer::is_equal(num_dir_obj - 1, num_assays)) { // -1 to account for the names.json file itself.
                throw std::runtime_error("more objects than expected inside the subdirectory");
            }
        } catch (...) {
            std::throw_with_nested(std::runtime_error("failed to validate 'assays'"));
        }
    }

    auto rd_path = path / "row_data";
    if (std::filesystem::exists(rd_path)) {
        try {
            auto rdmeta = read_object_metadata(rd_path);
            if (!satisfies_interface(rdmeta.type, "DATA_FRAME", options)) {
                throw std::runtime_error("object should satisfy the 'DATA_FRAME' interface");
            }

            validate(rd_path, rdmeta, options);
            if (height(rd_path, rdmeta, options) != num_rows) {
                throw std::runtime_error("data frame should have number of rows equal to that of the '" + metadata.type + "'");
            }
        } catch (...) {
            std::throw_with_nested(std::runtime_error("failed to validate 'row_data'"));
        }
    }

    auto cd_path = path / "column_data";
    if (std::filesystem::exists(cd_path)) {
        try {
            auto cdmeta = read_object_metadata(cd_path);
            if (!satisfies_interface(cdmeta.type, "DATA_FRAME", options)) {
                throw std::runtime_error("object should satisfy the 'DATA_FRAME' interface");
            }

            validate(cd_path, cdmeta, options);
            if (height(cd_path, cdmeta, options) != num_cols) {
                throw std::runtime_error("data frame should have number of rows equal to the number of columns of its parent '" + metadata.type + "'");
            }
        } catch (...) {
            std::throw_with_nested(std::runtime_error("failed to validate 'column_data'"));
        }
    }

    validate_metadata(path, "other_data", options);
}

/**
 * @param path Path to a directory containing a summarized experiment.
 * @param metadata Metadata for the object, typically read from its `OBJECT` file.
 * @param options Validation options.
 * @return Number of rows in the summarized experiment.
 */
inline std::size_t height_of_summarized_experiment([[maybe_unused]] const std::filesystem::path& path, const ObjectMetadata& metadata, [[maybe_unused]] const Options& options) {
    const std::string type_name = "summarized_experiment"; // use a separate variable to avoid dangling reference warnings from GCC.
    // Assume it's all valid, so we go straight for the kill.
    const auto& semap = extract_json_object(metadata.other, type_name);
    auto dims = extract_summarized_experiment_dimensions(semap);
    return dims.first;
}

/**
 * @param path Path to a directory containing a summarized experiment.
 * @param metadata Metadata for the object, typically read from its `OBJECT` file.
 * @param options Validation options.
 * @return A vector of length 2 containing the dimensions of the summarized experiment.
 */
inline std::vector<std::size_t> dimensions_of_summarized_experiment([[maybe_unused]] const std::filesystem::path& path, const ObjectMetadata& metadata, [[maybe_unused]] const Options& options) {
    const std::string type_name = "summarized_experiment"; // use a separate variable to avoid dangling reference warnings from GCC.
    // Assume it's all valid, so we go straight for the kill.
    const auto& semap = extract_json_object(metadata.other, type_name);
    auto dims = extract_summarized_experiment_dimensions(semap);
    return std::vector<std::size_t>{ dims.first, dims.second };
}

}

#endif
