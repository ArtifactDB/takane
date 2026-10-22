#ifndef TAKANE_MULTI_SAMPLE_DATASET_HPP
#define TAKANE_MULTI_SAMPLE_DATASET_HPP

#include "H5Cpp.h"
#include "ritsuko/ritsuko.hpp"
#include "sanisizer/sanisizer.hpp"

#include <filesystem>
#include <string>
#include <cstdint>
#include <stdexcept>
#include <exception>
#include <cstddef>

#include "utils_public.hpp"
#include "utils_other.hpp"
#include "utils_summarized_experiment.hpp"

/**
 * @file multi_sample_dataset.hpp
 * @brief Validation for multi-sample datasets.
 */

namespace takane {

/**
 * @cond
 */
void validate(const std::filesystem::path&, const ObjectMetadata&, const Options&);
std::size_t height(const std::filesystem::path&, const ObjectMetadata&, const Options&);
bool satisfies_interface(const std::string&, const std::string&, const Options&);
/**
 * @endcond
 */

/**
 * @param path Path to the directory containing the multi-sample dataset.
 * @param metadata Metadata for the object, typically read from its `OBJECT` file.
 * @param options Validation options.
 */
inline void validate_multi_sample_dataset(const std::filesystem::path& path, const ObjectMetadata& metadata, const Options& options) {
    const std::string type_name = "multi_sample_dataset"; // use a separate variable to avoid dangling reference warnings from GCC.

    try {
        const auto& dmap = extract_json_object(metadata.other, type_name);
        try {
            const std::string& vstring = extract_json_version_string(dmap);
            auto version = ritsuko::parse_version_string(vstring.c_str(), vstring.size(), /* skip_patch = */ true);
            if (version.major != 1) {
                throw std::runtime_error("unsupported version string '" + vstring + "'");
            }
        } catch (...) {
            std::throw_with_nested(std::runtime_error("failed to read 'version'"));
        }
    } catch (...) {
        std::throw_with_nested(std::runtime_error("failed to read '" + type_name + "' in object metadata"));
    }

    // Sample data should exist.
    std::size_t num_samples;
    try {
        auto sd_path = path / "sample_data";
        auto sdmeta = read_object_metadata(sd_path);
        if (!satisfies_interface(sdmeta.type, "DATA_FRAME", options)) {
            throw std::runtime_error("object in 'sample_data' should satisfy the 'DATA_FRAME' interface");
        }
        validate(sd_path, sdmeta, options);
        num_samples = height(sd_path, sdmeta, options);
    } catch (...) {
        std::throw_with_nested(std::runtime_error("failed to validate 'sample_data'"));
    }

    // Checking the experiments.
    std::vector<size_t> num_columns;
    auto edir = path / "experiments";
    if (std::filesystem::exists(edir)) {
        try {
            const auto num_experiments = [&]{
                try {
                    return extract_summarized_experiment_names(edir / "names.json").size(); 
                } catch (...) {
                    std::throw_with_nested(std::runtime_error("failed to read 'names.json'"));
                }
            }();

            num_columns.reserve(num_experiments);
            for (I<decltype(num_experiments)> e = 0; e < num_experiments; ++e) {
                auto ename = ritsuko::itoa(e);
                auto epath = edir / ename;
                try {
                    auto emeta = read_object_metadata(epath);
                    if (!satisfies_interface(emeta.type, "SUMMARIZED_EXPERIMENT", options)) {
                        throw std::runtime_error("object should satisfy the 'SUMMARIZED_EXPERIMENT' interface");
                    }
                    validate(epath, emeta, options);
                    auto dims = dimensions(epath, emeta, options);
                    num_columns.push_back(dims[1]);
                } catch (...) {
                    std::throw_with_nested(std::runtime_error("failed to validate experiment " + ename));
                }
            }

            const auto num_dir_obj = count_directory_entries(edir);
            if (!sanisizer::is_equal(num_dir_obj - 1, num_experiments)) { // -1 to account for the names.json file itself.
                throw std::runtime_error("more objects than expected inside the subdirectory");
            }
        } catch (...) {
            std::throw_with_nested(std::runtime_error("failed to validate 'experiments'"));
        }
    }

    // Checking the sample map, if it exists or if there are any experiments.
    const auto mappath = path / "sample_map.h5";
    if (!num_columns.empty() || std::filesystem::exists(mappath)) {
        try {
            H5::H5File handle(mappath, H5F_ACC_RDONLY);
            auto ghandle = handle.openGroup(type_name);

            const auto num_experiments = num_columns.size();
            for (I<decltype(num_experiments)> e = 0; e < num_experiments; ++e) {
                auto ename = ritsuko::itoa(e);
                try {
                    auto dhandle = ghandle.openDataSet(ename);
                    if (ritsuko::hdf5::exceeds_integer_limit(dhandle, 64, false)) {
                        throw std::runtime_error("expected a datatype that fits into a 64-bit unsigned integer");
                    }

                    auto dspace = dhandle.getSpace();
                    if (dspace.getSimpleExtentNdims() != 1) {
                        throw std::runtime_error("expected a 1-dimensional dataset");
                    }
                    hsize_t len;
                    dspace.getSimpleExtentDims(&len);
                    if (!sanisizer::is_equal(len, num_columns[e])) {
                        throw std::runtime_error("dataset extent should equal the number of columns of 'experiments/" + ename + "'");
                    }

                    ritsuko::hdf5::Stream1dNumericDataset<std::uint64_t> stream(
                        &dhandle,
                        len,
                        [&]{
                            ritsuko::hdf5::Stream1dNumericDatasetOptions opt;
                            opt.contiguous_chunk_size = options.hdf5_buffer_size;
                            return opt;
                        }()
                    );
                    iterate_stream<std::uint64_t>(
                        stream,
                        [&](hsize_t, std::uint64_t val) -> void {
                            if (sanisizer::is_greater_than_or_equal(val, num_samples)) {
                                throw std::runtime_error("entries should be less than the number of samples");
                            }
                        }
                    );
                } catch (...) {
                    std::throw_with_nested(std::runtime_error("failed to validate mapping for experiment " + ename));
                }
            }

            if (!sanisizer::is_equal(num_columns.size(), ghandle.getNumObjs())) {
                throw std::runtime_error("more objects present in the 'multi_sample_dataset' group than expected");
            }
        } catch (...) {
            std::throw_with_nested(std::runtime_error("failed to validate the sample mapping in 'sample_map.h5'"));
        }
    }

    validate_metadata(path, "other_data", options);
}

}

#endif
