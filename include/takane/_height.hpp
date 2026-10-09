#ifndef TAKANE_HEIGHT_HPP
#define TAKANE_HEIGHT_HPP

#include <functional>
#include <string>
#include <stdexcept>
#include <filesystem>
#include <cstddef>

#include "utils_public.hpp"
#include "atomic_vector.hpp"
#include "atomic_vector_list.hpp"
#include "compressed_sparse_matrix.hpp"
#include "data_frame_list.hpp"
#include "data_frame.hpp"
#include "delayed_array.hpp"
#include "dense_array.hpp"
#include "genomic_ranges.hpp"
#include "genomic_ranges_list.hpp"
#include "simple_list.hpp"
#include "string_factor.hpp"
#include "summarized_experiment.hpp"
//#include "data_frame_factor.hpp"
//#include "sequence_string_set.hpp"
//#include "bumpy_atomic_array.hpp"
//#include "bumpy_data_frame_array.hpp"
//#include "vcf_experiment.hpp"

/**
 * @file _height.hpp
 * @brief Dispatch to functions for the object's height.
 */

namespace takane {

/**
 * @cond
 */
inline auto default_height_registry() {
    std::unordered_map<std::string, std::function<std::size_t(const std::filesystem::path&, const ObjectMetadata& m, const Options& os)> > registry;
    registry["atomic_vector"] = height_of_atomic_vector;
    registry["atomic_vector_list"] = height_of_atomic_vector_list;
    registry["compressed_sparse_matrix"] = height_of_compressed_sparse_matrix;
    registry["data_frame"] = height_of_data_frame;
    registry["data_frame_factor"] = height_of_data_frame_factor;
    registry["data_frame_list"] = height_of_data_frame_list;
    registry["delayed_array"] = height_of_delayed_array;
    registry["dense_array"] = height_of_dense_array;
    registry["genomic_ranges"] = height_of_genomic_ranges;
    registry["genomic_ranges_list"] = height_of_genomic_ranges_list;
    registry["simple_list"] = height_of_simple_list;
    registry["string_factor"] = height_of_string_factor;
    registry["summarized_experiment"] = height_of_summarized_experiment;

    // Subclasses of the SE, so we just re-use its methods here.
    registry["ranged_summarized_experiment"] = height_of_summarized_experiment;
    registry["single_cell_experiment"] = height_of_summarized_experiment;
    registry["spatial_experiment"] = height_of_summarized_experiment;

//    registry["sequence_string_set"] = sequence_string_set::height;
//    registry["bumpy_atomic_array"] = bumpy_atomic_array::height;
//    registry["bumpy_data_frame_array"] = bumpy_data_frame_array::height;
//    registry["vcf_experiment"] = vcf_experiment::height;
    return registry;
} 
/**
 * @endcond
 */

/**
 * Get the height of an object in a subdirectory, based on the supplied object type.
 *
 * `height()` is used to check the shape of objects stored in vertical containers, e.g., columns of a `data_frame`.
 * For vectors or other 1-dimensional objects, the height is usually just the length of the object (for some object-specific definition of "length").
 * For higher-dimensional objects, the height is usually the extent of the first dimension.
 *
 * Applications can supply custom height functions for a given type via `Options::custom_height`.
 * If available, the supplied custom function will be used instead of the default.
 *
 * @param path Path to a directory representing an object.
 * @param metadata Metadata for the object, typically determined from its `OBJECT` file.
 * @param options Validation options.
 *
 * @return The object's height.
 */
inline std::size_t height(const std::filesystem::path& path, const ObjectMetadata& metadata, const Options& options) {
    auto cIt = options.custom_height.find(metadata.type);
    if (cIt != options.custom_height.end()) {
        return (cIt->second)(path, metadata, options);
    }

    static const auto height_registry = default_height_registry();
    auto vrIt = height_registry.find(metadata.type);
    if (vrIt == height_registry.end()) {
        throw std::runtime_error("no registered 'height' function for object type '" + metadata.type + "' at '" + path.string() + "'");
    }

    return (vrIt->second)(path, metadata, options);
}

/**
 * Get the height of an object in a subdirectory, using its `OBJECT` file to automatically determine the type.
 *
 * @param path Path to a directory containing an object.
 * @param options Validation options.
 * @return The object's height.
 */
inline std::size_t height(const std::filesystem::path& path, const Options& options) {
    return height(path, read_object_metadata(path), options);
}

/**
 * Overload of `height()` with default settings.
 *
 * @param path Path to a directory containing an object.
 * @return The object's height.
 */
inline std::size_t height(const std::filesystem::path& path) {
    Options options;
    return height(path, options);
}

}

#endif
