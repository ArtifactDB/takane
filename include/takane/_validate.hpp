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
#include "delayed_array.hpp"
#include "dense_array.hpp"
#include "fasta_file.hpp"
#include "fastq_file.hpp"
#include "genomic_ranges.hpp"
#include "genomic_ranges_list.hpp"
#include "gff_file.hpp"
#include "gmt_file.hpp"
#include "image_file.hpp"
#include "ranged_summarized_experiment.hpp"
#include "rds_file.hpp"
#include "sequence_information.hpp"
#include "simple_list.hpp"
#include "string_factor.hpp"
#include "summarized_experiment.hpp"
//#include "single_cell_experiment.hpp"
//#include "spatial_experiment.hpp"
//#include "multi_sample_dataset.hpp"
//#include "sequence_string_set.hpp"
//#include "bumpy_atomic_array.hpp"
//#include "bumpy_data_frame_array.hpp"
//#include "vcf_experiment.hpp"

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
    registry["atomic_vector"] = validate_atomic_vector;
    registry["atomic_vector_list"] = validate_atomic_vector_list;
    registry["bam_file"] = validate_bam_file;
    registry["bcf_file"] = validate_bcf_file;
    registry["bed_file"] = validate_bed_file;
    registry["bigbed_file"] = validate_bigbed_file;
    registry["bigwig_file"] = validate_bigwig_file;
    registry["compressed_sparse_matrix"] = validate_compressed_sparse_matrix;
    registry["data_frame"] = validate_data_frame;
    registry["data_frame_factor"] = validate_data_frame_factor;
    registry["data_frame_list"] = validate_data_frame_list;
    registry["delayed_array"] = validate_delayed_array;
    registry["dense_array"] = validate_dense_array;
    registry["fasta_file"] = validate_fasta_file;
    registry["fastq_file"] = validate_fastq_file;
    registry["genomic_ranges"] = validate_genomic_ranges;
    registry["genomic_ranges_list"] = validate_genomic_ranges_list;
    registry["gff_file"] = validate_gff_file;
    registry["gmt_file"] = validate_gmt_file;
    registry["image_file"] = validate_image_file;
    registry["ranged_summarized_experiment"] = validate_ranged_summarized_experiment;
    registry["rds_file"] = validate_rds_file;
    registry["sequence_information"] = validate_sequence_information;
    registry["simple_list"] = validate_simple_list;
    registry["string_factor"] = validate_string_factor;
    registry["summarized_experiment"] = validate_summarized_experiment;
//    registry["single_cell_experiment"] = validate;
//    registry["spatial_experiment"] = validate;
//    registry["multi_sample_dataset"] = validate;
//    registry["sequence_string_set"] = validate;
//    registry["bumpy_atomic_array"] = validate;
//    registry["bumpy_data_frame_array"] = validate;
//    registry["vcf_experiment"] = validate;
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
