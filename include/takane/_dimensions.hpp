#ifndef TAKANE_DIMENSIONS_HPP
#define TAKANE_DIMENSIONS_HPP

#include <functional>
#include <string>
#include <stdexcept>
#include <filesystem>
#include <vector>

#include "compressed_sparse_matrix.hpp"
#include "data_frame.hpp"
#include "delayed_array.hpp"
#include "dense_array.hpp"
#include "summarized_experiment.hpp"
//#include "bumpy_atomic_array.hpp"
//#include "bumpy_data_frame_array.hpp"
//#include "vcf_experiment.hpp"

/**
 * @file _dimensions.hpp
 * @brief Dispatch to functions for the object's dimensions.
 */

namespace takane {

/**
 * @cond
 */
inline auto default_dimensions_registry() {
    typedef std::vector<std::size_t> Dims;
    std::unordered_map<std::string, std::function<Dims(const std::filesystem::path&, const ObjectMetadata&, const Options& os)> > registry;

    registry["compressed_sparse_matrix"] = dimensions_of_compressed_sparse_matrix;
    registry["data_frame"] = dimensions_of_data_frame;
    registry["delayed_array"] = dimensions_of_delayed_array;
    registry["dense_array"] = dimensions_of_dense_array;
    registry["ranged_summarized_experiment"] = dimensions_of_summarized_experiment; // subclass of an SE.
    registry["single_cell_experiment"] = dimensions_of_summarized_experiment; // subclass of an SE.
    registry["summarized_experiment"] = dimensions_of_summarized_experiment;
    registry["spatial_experiment"] = dimensions_of_summarized_experiment; // subclass of an SE.

//    registry["bumpy_atomic_array"] = bumpy_atomic_array::dimensions;
//    registry["bumpy_data_frame_array"] = bumpy_data_frame_array::dimensions;
//    registry["vcf_experiment"] = vcf_experiment::dimensions;

    return registry;
} 
/**
 * @endcond
 */

/**
 * Get the dimensions of a multi-dimensional object in a subdirectory, based on the supplied object type.
 *
 * Applications can supply custom dimension functions for a given type via `Options::custom_dimensions`.
 * If available, the supplied custom function will be used instead of the default.
 *
 * @param path Path to a directory representing an object.
 * @param metadata Metadata for the object, typically determined from its `OBJECT` file.
 * @param options Validation options.
 *
 * @return Vector containing the object's dimensions.
 */
inline std::vector<std::size_t> dimensions(const std::filesystem::path& path, const ObjectMetadata& metadata, const Options& options) {
    auto cIt = options.custom_dimensions.find(metadata.type);
    if (cIt != options.custom_dimensions.end()) {
        return (cIt->second)(path, metadata, options);
    }

    static const auto dimensions_registry = default_dimensions_registry();
    auto vrIt = dimensions_registry.find(metadata.type);
    if (vrIt == dimensions_registry.end()) {
        throw std::runtime_error("no registered 'dimensions' function for object type '" + metadata.type + "' at '" + path.string() + "'");
    }

    return (vrIt->second)(path, metadata, options);
}

/**
 * Get the dimensions of an object in a subdirectory, using its `OBJECT` file to automatically determine the type.
 *
 * @param path Path to a directory containing an object.
 * @param options Validation options.
 * @return The object's dimensions.
 */
inline std::vector<size_t> dimensions(const std::filesystem::path& path, const Options& options) {
    return dimensions(path, read_object_metadata(path), options);
}

/**
 * Overload of `dimensions()` with default settings.
 *
 * @param path Path to a directory containing an object.
 * @return The object's dimensions.
 */
inline std::vector<size_t> dimensions(const std::filesystem::path& path) {
    Options options;
    return dimensions(path, options);
}

}

#endif
