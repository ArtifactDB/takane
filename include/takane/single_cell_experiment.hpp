#ifndef TAKANE_SINGLE_CELL_EXPERIMENT_HPP
#define TAKANE_SINGLE_CELL_EXPERIMENT_HPP


#include "summarized_experiment.hpp"
#include "ranged_summarized_experiment.hpp"

#include "utils_public.hpp"
#include "utils_summarized_experiment.hpp"

#include <filesystem>
#include <stdexcept>
#include <exception>
#include <unordered_set>
#include <string>
#include <cstddef>

/**
 * @file single_cell_experiment.hpp
 * @brief Validation for single cell experiments.
 */

namespace takane {

/**
 * @cond
 */
void validate(const std::filesystem::path&, const ObjectMetadata&, const Options& options);
std::vector<std::size_t> dimensions(const std::filesystem::path&, const ObjectMetadata&, const Options& options);
bool satisfies_interface(const std::string&, const std::string&, const Options&);
/**
 * @endcond
 */

/**
 * @param path Path to the directory containing the single cell experiment.
 * @param metadata Metadata for the object, typically read from its `OBJECT` file.
 * @param options Validation options.
 */
inline void validate_single_cell_experiment(const std::filesystem::path& path, const ObjectMetadata& metadata, const Options& options) {
    validate_ranged_summarized_experiment(path, metadata, options);
    auto num_cols = dimensions_of_summarized_experiment(path, metadata, options)[1];

    const std::string type_name = "single_cell_experiment"; // use a separate variable to avoid dangling reference warnings from GCC.
    std::optional<std::string> main_exp_name;
    try {
        const auto& scemap = extract_json_object(metadata.other, type_name);
        try {
            const std::string& vstring = extract_json_version_string(scemap);
            auto version = ritsuko::parse_version_string(vstring.c_str(), vstring.size(), /* skip_patch = */ true);
            if (version.major != 1) {
                throw std::runtime_error("unsupported version string '" + vstring + "'");
            }
        } catch (...) {
            std::throw_with_nested(std::runtime_error("failed to read 'version'"));
        }

        // Validating the main experiment name.
        auto mIt = scemap.find("main_experiment_name");
        if (mIt != scemap.end()) {
            try {
                const auto& ver = mIt->second;
                if (ver->type() != millijson::STRING) {
                    throw std::runtime_error("expected property to be a string");
                }
                auto& mname = reinterpret_cast<const millijson::String*>(ver.get())->value();
                if (mname.empty()) {
                    throw std::runtime_error("expected a non-empty string");
                }
                main_exp_name = std::move(mname);
            } catch (...) {
                std::throw_with_nested(std::runtime_error("failed to read 'main_experiment_name'"));
            }
        }
    } catch (...) {
        std::throw_with_nested(std::runtime_error("failed to validate '" + type_name + "' in object metadata"));
    }

    // Check the reduced dimensions.
    auto rddir = path / "reduced_dimensions";
    if (std::filesystem::exists(rddir)) {
        try {
            auto num_rd = [&]{
                try {
                    return extract_summarized_experiment_names(rddir / "names.json").size();
                } catch (...) {
                    std::throw_with_nested(std::runtime_error("failed to validate 'names.json'"));
                }
            }();

            for (I<decltype(num_rd)> i = 0; i < num_rd; ++i) {
                auto rdname = std::to_string(i);
                auto rdpath = rddir / rdname;

                try {
                    auto rdmeta = read_object_metadata(rdpath);
                    validate(rdpath, rdmeta, options);

                    auto dims = dimensions(rdpath, rdmeta, options);
                    if (dims.size() < 1) {
                        throw std::runtime_error("object should have at least one dimension");
                    }
                    if (dims[0] != num_cols) {
                        throw std::runtime_error("object should have the same number of rows as the columns of its parent '" + metadata.type + "'");
                    }
                } catch (...) {
                    std::throw_with_nested(std::runtime_error("failed to validate reduced dimensions " + rdname));
                }
            }

            auto num_dir_obj = count_directory_entries(rddir);
            if (!sanisizer::is_equal(num_dir_obj - 1, num_rd)) { // -1 to account for the names.json file itself.
                throw std::runtime_error("more objects than expected inside the subdirectory");
            }
        } catch (...) {
            std::throw_with_nested(std::runtime_error("failed to validate 'reduced_dimensions'"));
        }
    }

    // Check the alternative experiments.
    auto aedir = path / "alternative_experiments";
    if (std::filesystem::exists(aedir)) {
        try {
            const auto num_ae = [&]{
                try {
                    auto alt_names = extract_summarized_experiment_names(aedir / "names.json");
                    if (main_exp_name.has_value()) {
                        for (const auto& ae_name : alt_names) {
                            if (ae_name == *main_exp_name) {
                                throw std::runtime_error("main experiment name '" + *main_exp_name + "' should not be present");
                            }
                        }
                    }
                    return alt_names.size();
                } catch (...) {
                    std::throw_with_nested(std::runtime_error("failed to validate 'names.json'"));
                }
            }();

            for (I<decltype(num_ae)> i = 0; i < num_ae; ++i) {
                auto aename = std::to_string(i);
                auto aepath = aedir / aename;

                try {
                    auto aemeta = read_object_metadata(aepath);
                    if (!satisfies_interface(aemeta.type, "SUMMARIZED_EXPERIMENT", options)) {
                        throw std::runtime_error("object should satisfy the 'SUMMARIZED_EXPERIMENT' interface");
                    }

                    validate(aepath, aemeta, options);
                    auto dims = dimensions(aepath, aemeta, options);
                    if (dims[1] != num_cols) {
                        throw std::runtime_error("object should have the same number of columns as its parent '" + metadata.type + "'");
                    }
                } catch (...) {
                    std::throw_with_nested(std::runtime_error("failed to validate 'names.json'"));
                }
            }

            auto num_dir_obj = count_directory_entries(aedir);
            if (!sanisizer::is_equal(num_dir_obj - 1, num_ae)) { // -1 to account for the names.json file itself.
                throw std::runtime_error("more objects than expected inside the subdirectory");
            }
        } catch (...) {
            std::throw_with_nested(std::runtime_error("failed to validate 'alternative_experiments'"));
        }
    }
}

}

#endif
