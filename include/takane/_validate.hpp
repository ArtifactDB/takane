#ifndef TAKANE_VALIDATE_HPP
#define TAKANE_VALIDATE_HPP

#include <functional>
#include <string>
#include <stdexcept>
#include <filesystem>

#include "utils_public.hpp"

#include "atomic_vector.hpp"
#include "atomic_vector_list.hpp"
#include "bam_file.hpp"
#include "bcf_file.hpp"
#include "bed_file.hpp"
#include "bigbed_file.hpp"
#include "bigwig_file.hpp"
#include "compressed_sparse_matrix.hpp"
#include "data_frame.hpp"
#include "data_frame_factor.hpp"
#include "data_frame_list.hpp"
#include "dense_array.hpp"
#include "fasta_file.hpp"
#include "fastq_file.hpp"
#include "gff_file.hpp"
#include "gmt_file.hpp"
#include "image_file.hpp"
#include "rds_file.hpp"
#include "simple_list.hpp"
#include "string_factor.hpp"
//#include "sequence_information.hpp"
//#include "genomic_ranges.hpp"
//#include "genomic_ranges_list.hpp"
//#include "summarized_experiment.hpp"
//#include "ranged_summarized_experiment.hpp"
//#include "single_cell_experiment.hpp"
//#include "spatial_experiment.hpp"
//#include "multi_sample_dataset.hpp"
//#include "sequence_string_set.hpp"
//#include "bumpy_atomic_array.hpp"
//#include "bumpy_data_frame_array.hpp"
//#include "vcf_experiment.hpp"
//#include "delayed_array.hpp"

/**
 * @file _validate.hpp
 * @brief Validation dispatch function.
 */

namespace takane {

/**
 * @cond
 */
inline auto default_validate_registry() {
    std::unordered_map<std::string, std::function<void(const std::filesystem::path&, const ObjectMetadata&, const Options&)> > registry;
    registry["atomic_vector"] = [](const std::filesystem::path& p, const ObjectMetadata& m, const Options& o) -> void { validate_atomic_vector(p, m, o); };
    registry["atomic_vector_list"] = [](const std::filesystem::path& p, const ObjectMetadata& m, const Options& o) -> void { validate_atomic_vector_list(p, m, o); };
    registry["bam_file"] = [](const std::filesystem::path& p, const ObjectMetadata& m, const Options& o) -> void { validate_bam_file(p, m, o); };
    registry["bcf_file"] = [](const std::filesystem::path& p, const ObjectMetadata& m, const Options& o) -> void { validate_bcf_file(p, m, o); };
    registry["bed_file"] = [](const std::filesystem::path& p, const ObjectMetadata& m, const Options& o) -> void { validate_bed_file(p, m, o); };
    registry["bigbed_file"] = [](const std::filesystem::path& p, const ObjectMetadata& m, const Options& o) -> void { validate_bigbed_file(p, m, o); };
    registry["bigwig_file"] = [](const std::filesystem::path& p, const ObjectMetadata& m, const Options& o) -> void { validate_bigwig_file(p, m, o); };
    registry["compressed_sparse_matrix"] = [](const std::filesystem::path& p, const ObjectMetadata& m, const Options& o) -> void { validate_compressed_sparse_matrix(p, m, o); };
    registry["data_frame"] = [](const std::filesystem::path& p, const ObjectMetadata& m, const Options& o) -> void { validate_data_frame(p, m, o); };
    registry["data_frame_factor"] = [](const std::filesystem::path& p, const ObjectMetadata& m, const Options& o) -> void { validate_data_frame_factor(p, m, o); };
    registry["data_frame_list"] = [](const std::filesystem::path& p, const ObjectMetadata& m, const Options& o) -> void { validate_data_frame_list(p, m, o); };
    registry["dense_array"] = [](const std::filesystem::path& p, const ObjectMetadata& m, const Options& o) -> void { validate_dense_array(p, m, o); };
    registry["fasta_file"] = [](const std::filesystem::path& p, const ObjectMetadata& m, const Options& o) -> void { validate_fasta_file(p, m, o); };
    registry["fastq_file"] = [](const std::filesystem::path& p, const ObjectMetadata& m, const Options& o) -> void { validate_fastq_file(p, m, o); };
    registry["gff_file"] = [](const std::filesystem::path& p, const ObjectMetadata& m, const Options& o) -> void { validate_gff_file(p, m, o); };
    registry["gmt_file"] = [](const std::filesystem::path& p, const ObjectMetadata& m, const Options& o) -> void { validate_gmt_file(p, m, o); };
    registry["simple_list"] = [](const std::filesystem::path& p, const ObjectMetadata& m, const Options& o) -> void { validate_simple_list(p, m, o); };
    registry["string_factor"] = [](const std::filesystem::path& p, const ObjectMetadata& m, const Options& o) -> void { validate_string_factor(p, m, o); };
//    registry["sequence_information"] = [](const std::filesystem::path& p, const ObjectMetadata& m, Options& o) { sequence_information::validate(p, m, o); };
//    registry["genomic_ranges"] = [](const std::filesystem::path& p, const ObjectMetadata& m, Options& o) { genomic_ranges::validate(p, m, o); };
//    registry["genomic_ranges_list"] = [](const std::filesystem::path& p, const ObjectMetadata& m, Options& o) { genomic_ranges_list::validate(p, m, o); };
//    registry["summarized_experiment"] = [](const std::filesystem::path& p, const ObjectMetadata& m, Options& o) { summarized_experiment::validate(p, m, o); };
//    registry["ranged_summarized_experiment"] = [](const std::filesystem::path& p, const ObjectMetadata& m, Options& o) { ranged_summarized_experiment::validate(p, m, o); };
//    registry["single_cell_experiment"] = [](const std::filesystem::path& p, const ObjectMetadata& m, Options& o) { single_cell_experiment::validate(p, m, o); };
//    registry["spatial_experiment"] = [](const std::filesystem::path& p, const ObjectMetadata& m, Options& o) { spatial_experiment::validate(p, m, o); };
//    registry["multi_sample_dataset"] = [](const std::filesystem::path& p, const ObjectMetadata& m, Options& o) { multi_sample_dataset::validate(p, m, o); };
//    registry["sequence_string_set"] = [](const std::filesystem::path& p, const ObjectMetadata& m, Options& o) { sequence_string_set::validate(p, m, o); };
//    registry["rds_file"] = [](const std::filesystem::path& p, const ObjectMetadata& m, Options& o) { rds_file::validate(p, m, o); };
//    registry["bumpy_atomic_array"] = [](const std::filesystem::path& p, const ObjectMetadata& m, Options& o) { bumpy_atomic_array::validate(p, m, o); };
//    registry["bumpy_data_frame_array"] = [](const std::filesystem::path& p, const ObjectMetadata& m, Options& o) { bumpy_data_frame_array::validate(p, m, o); };
//    registry["vcf_experiment"] = [](const std::filesystem::path& p, const ObjectMetadata& m, Options& o) { vcf_experiment::validate(p, m, o); };
//    registry["delayed_array"] = [](const std::filesystem::path& p, const ObjectMetadata& m, Options& o) { delayed_array::validate(p, m, o); };
//    registry["image_file"] = [](const std::filesystem::path& p, const ObjectMetadata& m, Options& o) { image_file::validate(p, m, o); };
    return registry;
} 
/**
 * @endcond
 */

/**
 * Validate an object in a subdirectory, based on the supplied object type.
 *
 * Applications can supply custom validation functions for a given type via `Options::custom_validate`.
 * If available, the supplied custom function will be used instead of the default.
 *
 * @param path Path to a directory representing an object.
 * @param metadata Metadata for the object, typically determined from its `OBJECT` file.
 * @param options Validation options.
 */
inline void validate(const std::filesystem::path& path, const ObjectMetadata& metadata, const Options& options) {
    auto cIt = options.custom_validate.find(metadata.type);

    if (cIt != options.custom_validate.end()) {
        try {
            (cIt->second)(path, metadata, options);
        } catch (std::exception& e) {
            std::throw_with_nested(std::runtime_error("failed to validate '" + metadata.type + "' object"));
        }

    } else {
        static const auto validate_registry = default_validate_registry();
        auto vrIt = validate_registry.find(metadata.type);
        if (vrIt == validate_registry.end()) {
            throw std::runtime_error("no registered validation function for object type '" + metadata.type + "'");
        }

        // Can't easily roll this out, as this is const and the above is not.
        try {
            (vrIt->second)(path, metadata, options);
        } catch (std::exception& e) {
            std::throw_with_nested(std::runtime_error("failed to validate '" + metadata.type + "' object"));
        }
    }

    if (options.custom_global_validate) {
        try {
            options.custom_global_validate(path, metadata, options);
        } catch (std::exception& e) {
            std::throw_with_nested(std::runtime_error("failed additional validation for '" + metadata.type + "'"));
        }
    }
}

/**
 * Validate an object in a subdirectory, using its `OBJECT` file to automatically determine the type.
 *
 * @param path Path to a directory containing an object.
 * @param options Validation options.
 */
inline void validate(const std::filesystem::path& path, const Options& options) {
    validate(path, read_object_metadata(path), options);
}

/**
 * Overload of `validate()` with default settings.
 *
 * @param path Path to a directory containing an object.
 */
inline void validate(const std::filesystem::path& path) {
    Options options;
    validate(path, options);
}

}

#endif
