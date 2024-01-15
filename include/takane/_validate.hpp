#ifndef TAKANE_VALIDATE_HPP
#define TAKANE_VALIDATE_HPP

#include <functional>
#include <string>
#include <stdexcept>
#include <filesystem>

#include "utils_public.hpp"
#include "atomic_vector.hpp"
#include "string_factor.hpp"
#include "simple_list.hpp"
#include "data_frame.hpp"
#include "data_frame_factor.hpp"
#include "sequence_information.hpp"
#include "genomic_ranges.hpp"
#include "atomic_vector_list.hpp"
#include "data_frame_list.hpp"
#include "genomic_ranges_list.hpp"
#include "dense_array.hpp"
#include "compressed_sparse_matrix.hpp"
#include "summarized_experiment.hpp"
#include "ranged_summarized_experiment.hpp"
#include "single_cell_experiment.hpp"
#include "spatial_experiment.hpp"
#include "multi_sample_dataset.hpp"
#include "sequence_string_set.hpp"
#include "bam_file.hpp"
#include "bcf_file.hpp"
#include "bigwig_file.hpp"
#include "bigbed_file.hpp"
#include "fasta_file.hpp"
#include "fastq_file.hpp"
#include "bed_file.hpp"
#include "gmt_file.hpp"
#include "gff_file.hpp"
#include "bumpy_atomic_array.hpp"
#include "bumpy_data_frame_array.hpp"
#include "vcf_experiment.hpp"
#include "delayed_array.hpp"

/**
 * @file _validate.hpp
 * @brief Validation dispatch function.
 */

namespace takane {

/**
 * @cond
 */
namespace internal_validate {

inline ValidateRegistry default_registry() {
    ValidateRegistry registry;
    registry["atomic_vector"] = [](const std::filesystem::path& p, const ObjectMetadata& m, const Options& o, State& s) { atomic_vector::validate(p, m, o, s); };
    registry["string_factor"] = [](const std::filesystem::path& p, const ObjectMetadata& m, const Options& o, State& s) { string_factor::validate(p, m, o, s); };
    registry["simple_list"] = [](const std::filesystem::path& p, const ObjectMetadata& m, const Options& o, State& s) { simple_list::validate(p, m, o, s); };
    registry["data_frame"] = [](const std::filesystem::path& p, const ObjectMetadata& m, const Options& o, State& s) { data_frame::validate(p, m, o, s); };
    registry["data_frame_factor"] = [](const std::filesystem::path& p, const ObjectMetadata& m, const Options& o, State& s) { data_frame_factor::validate(p, m, o, s); };
    registry["sequence_information"] = [](const std::filesystem::path& p, const ObjectMetadata& m, const Options& o, State& s) { sequence_information::validate(p, m, o, s); };
    registry["genomic_ranges"] = [](const std::filesystem::path& p, const ObjectMetadata& m, const Options& o, State& s) { genomic_ranges::validate(p, m, o, s); };
    registry["atomic_vector_list"] = [](const std::filesystem::path& p, const ObjectMetadata& m, const Options& o, State& s) { atomic_vector_list::validate(p, m, o, s); };
    registry["data_frame_list"] = [](const std::filesystem::path& p, const ObjectMetadata& m, const Options& o, State& s) { data_frame_list::validate(p, m, o, s); };
    registry["genomic_ranges_list"] = [](const std::filesystem::path& p, const ObjectMetadata& m, const Options& o, State& s) { genomic_ranges_list::validate(p, m, o, s); };
    registry["dense_array"] = [](const std::filesystem::path& p, const ObjectMetadata& m, const Options& o, State& s) { dense_array::validate(p, m, o, s); };
    registry["compressed_sparse_matrix"] = [](const std::filesystem::path& p, const ObjectMetadata& m, const Options& o, State& s) { compressed_sparse_matrix::validate(p, m, o, s); };
    registry["summarized_experiment"] = [](const std::filesystem::path& p, const ObjectMetadata& m, const Options& o, State& s) { summarized_experiment::validate(p, m, o, s); };
    registry["ranged_summarized_experiment"] = [](const std::filesystem::path& p, const ObjectMetadata& m, const Options& o, State& s) { ranged_summarized_experiment::validate(p, m, o, s); };
    registry["single_cell_experiment"] = [](const std::filesystem::path& p, const ObjectMetadata& m, const Options& o, State& s) { single_cell_experiment::validate(p, m, o, s); };
    registry["spatial_experiment"] = [](const std::filesystem::path& p, const ObjectMetadata& m, const Options& o, State& s) { spatial_experiment::validate(p, m, o, s); };
    registry["multi_sample_dataset"] = [](const std::filesystem::path& p, const ObjectMetadata& m, const Options& o, State& s) { multi_sample_dataset::validate(p, m, o, s); };
    registry["sequence_string_set"] = [](const std::filesystem::path& p, const ObjectMetadata& m, const Options& o, State& s) { sequence_string_set::validate(p, m, o, s); };
    registry["bam_file"] = [](const std::filesystem::path& p, const ObjectMetadata& m, const Options& o, State& s) { bam_file::validate(p, m, o, s); };
    registry["bcf_file"] = [](const std::filesystem::path& p, const ObjectMetadata& m, const Options& o, State& s) { bcf_file::validate(p, m, o, s); };
    registry["bigwig_file"] = [](const std::filesystem::path& p, const ObjectMetadata& m, const Options& o, State& s) { bigwig_file::validate(p, m, o, s); };
    registry["bigbed_file"] = [](const std::filesystem::path& p, const ObjectMetadata& m, const Options& o, State& s) { bigbed_file::validate(p, m, o, s); };
    registry["fasta_file"] = [](const std::filesystem::path& p, const ObjectMetadata& m, const Options& o, State& s) { fasta_file::validate(p, m, o, s); };
    registry["fastq_file"] = [](const std::filesystem::path& p, const ObjectMetadata& m, const Options& o, State& s) { fastq_file::validate(p, m, o, s); };
    registry["bed_file"] = [](const std::filesystem::path& p, const ObjectMetadata& m, const Options& o, State& s) { bed_file::validate(p, m, o, s); };
    registry["gmt_file"] = [](const std::filesystem::path& p, const ObjectMetadata& m, const Options& o, State& s) { gmt_file::validate(p, m, o, s); };
    registry["gff_file"] = [](const std::filesystem::path& p, const ObjectMetadata& m, const Options& o, State& s) { gff_file::validate(p, m, o, s); };
    registry["bumpy_atomic_array"] = [](const std::filesystem::path& p, const ObjectMetadata& m, const Options& o, State& s) { bumpy_atomic_array::validate(p, m, o, s); };
    registry["bumpy_data_frame_array"] = [](const std::filesystem::path& p, const ObjectMetadata& m, const Options& o, State& s) { bumpy_data_frame_array::validate(p, m, o, s); };
    registry["vcf_experiment"] = [](const std::filesystem::path& p, const ObjectMetadata& m, const Options& o, State& s) { vcf_experiment::validate(p, m, o, s); };
    registry["delayed_array"] = [](const std::filesystem::path& p, const ObjectMetadata& m, const Options& o, State& s) { delayed_array::validate(p, m, o, s); };
    return registry;
} 

}
/**
 * @endcond
 */

/**
 * Validate an object in a subdirectory, based on the supplied object type.
 *
 * Applications can supply custom validation functions for a given type via the `state.validate_registry`.
 * If available, the supplied custom function will be used instead of the default.
 *
 * @param path Path to a directory representing an object.
 * @param metadata Metadata for the object, typically determined from its `OBJECT` file.
 * @param options Validation options, mostly for input performance.
 * @param state Validation state, containing custom validation functions.
 */
inline void validate(const std::filesystem::path& path, const ObjectMetadata& metadata, const Options& options, State& state) {
    if (!std::filesystem::exists(path) || std::filesystem::status(path).type() != std::filesystem::file_type::directory) {
        throw std::runtime_error("expected '" + path.string() + "' to be a directory");
    }

    auto cIt = state.validate_registry.find(metadata.type);
    if (cIt != state.validate_registry.end()) {
        try {
            (cIt->second)(path, metadata, options, state);
        } catch (std::exception& e) {
            throw std::runtime_error("failed to validate '" + metadata.type + "' object at '" + path.string() + "'; " + std::string(e.what()));
        }
        return;
    }

    static const validate_registry = internal_validate::default_registry();
    auto vrIt = validate_registry.find(metadata.type);
    if (vrIt == validate_registry.end()) {
        throw std::runtime_error("no registered 'validate' function for object type '" + metadata.type + "' at '" + path.string() + "'");
    }

    try {
        (vrIt->second)(path, metadata, options, state);
    } catch (std::exception& e) {
        throw std::runtime_error("failed to validate '" + metadata.type + "' object at '" + path.string() + "'; " + std::string(e.what()));
    }
}

/**
 * Validate an object in a subdirectory, using its `OBJECT` file to automatically determine the type.
 *
 * @param path Path to a directory containing an object.
 * @param options Validation options, mostly for input performance.
 * @param state Validation state, containing custom validation functions.
 */
inline void validate(const std::filesystem::path& path, const Options& options, State& state) {
    validate(path, read_object_metadata(path), options);
}

/**
 * Overload of `validate()` with default settings.
 *
 * @param path Path to a directory containing an object.
 */
inline void validate(const std::filesystem::path& path) {
    State state;
    validate(path, Options(), state);
}

}

#endif
