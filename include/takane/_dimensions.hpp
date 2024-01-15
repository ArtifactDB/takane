#ifndef TAKANE_DIMENSIONS_HPP
#define TAKANE_DIMENSIONS_HPP

#include <functional>
#include <string>
#include <stdexcept>
#include <filesystem>
#include <vector>

#include "data_frame.hpp"
#include "dense_array.hpp"
#include "compressed_sparse_matrix.hpp"
#include "summarized_experiment.hpp"
#include "bumpy_atomic_array.hpp"
#include "bumpy_data_frame_array.hpp"
#include "vcf_experiment.hpp"

/**
 * @file _dimensions.hpp
 * @brief Dispatch to functions for the object's dimensions.
 */

namespace takane {

/**
 * @cond
 */
namespace internal_dimensions {

inline DimensionsRegistry default_registry() {
    DimensionsRegistry registry;
    typedef std::vector<size_t> Dims;

    registry["data_frame"] = [](const std::filesystem::path& p, const ObjectMetadata& m, const Options& o, State& s) -> Dims { return data_frame::dimensions(p, m, o, s); };
    registry["dense_array"] = [](const std::filesystem::path& p, const ObjectMetadata& m, const Options& o, State& s) -> Dims { return dense_array::dimensions(p, m, o, s); };
    registry["compressed_sparse_matrix"] = [](const std::filesystem::path& p, const ObjectMetadata& m, const Options& o, State& s) -> Dims { return compressed_sparse_matrix::dimensions(p, m, o, s); };

    // Subclasses of SE, so we just re-use the SE methods here.
    registry["summarized_experiment"] = [](const std::filesystem::path& p, const ObjectMetadata& m, const Options& o, State& s) -> Dims { return summarized_experiment::dimensions(p, m, o, s); };
    registry["ranged_summarized_experiment"] = [](const std::filesystem::path& p, const ObjectMetadata& m, const Options& o, State& s) -> Dims { return summarized_experiment::dimensions(p, m, o, s); };
    registry["single_cell_experiment"] = [](const std::filesystem::path& p, const ObjectMetadata& m, const Options& o, State& s) -> Dims { return summarized_experiment::dimensions(p, m, o, s); };
    registry["spatial_experiment"] = [](const std::filesystem::path& p, const ObjectMetadata& m, const Options& o, State& s) -> Dims { return summarized_experiment::dimensions(p, m, o, s); };

    registry["bumpy_atomic_array"] = [](const std::filesystem::path& p, const ObjectMetadata& m, const Options& o, State& s) -> Dims { return bumpy_atomic_array::dimensions(p, m, o, s); };
    registry["bumpy_data_frame_array"] = [](const std::filesystem::path& p, const ObjectMetadata& m, const Options& o, State& s) -> Dims { return bumpy_data_frame_array::dimensions(p, m, o, s); };
    registry["vcf_experiment"] = [](const std::filesystem::path& p, const ObjectMetadata& m, const Options& o, State& s) -> Dims { return vcf_experiment::dimensions(p, m, o, s); };
    registry["delayed_array"] = [](const std::filesystem::path& p, const ObjectMetadata& m, const Options& o, State& s) -> Dims { return delayed_array::dimensions(p, m, o, s); };

    return registry;
} 

}
/**
 * @endcond
 */

/**
 * Registry of functions to be used by `dimensions()`.
 * Applications can extend **takane** by adding new dimension functions for custom object types.
 */
inline DimensionsRegistry dimensions_registry = internal_dimensions::default_registry();

/**
 * Get the dimensions of a multi-dimensional object in a subdirectory, based on the supplied object type.
 *
 * Applications can supply custom dimension functions for a given type via the `state.dimensions_registry`.
 * If available, the supplied custom function will be used instead of the default.
 *
 * @param path Path to a directory representing an object.
 * @param metadata Metadata for the object, typically determined from its `OBJECT` file.
 * @param options Validation options, mostly for input performance.
 * @param state Validation state, containing custom dimension functions.
 *
 * @return Vector containing the object's dimensions.
 */
inline std::vector<size_t> dimensions(const std::filesystem::path& path, const ObjectMetadata& metadata, const Options& options, State& state) {
    auto cIt = state.dimensions_registry.find(metadata.type);
    if (cIt != state.dimensions_registry.end()) {
        return (cIt->second)(path, metadata, options, state);
    }

    static const dimensions_registry = internal_dimensions::default_registry();
    auto vrIt = dimensions_registry.find(metadata.type);
    if (vrIt == dimensions_registry.end()) {
        throw std::runtime_error("no registered 'dimensions' function for object type '" + metadata.type + "' at '" + path.string() + "'");
    }

    return (vrIt->second)(path, metadata, options, state);
}

/**
 * Get the dimensions of an object in a subdirectory, using its `OBJECT` file to automatically determine the type.
 *
 * @param path Path to a directory containing an object.
 * @param options Validation options, mostly for input performance.
 * @param state Validation state, containing custom dimension functions.
 * @return The object's dimensions.
 */
inline std::vector<size_t> dimensions(const std::filesystem::path& path, const Options& options, State& state) {
    return dimensions(path, read_object_metadata(path), options, state);
}

/**
 * Overload of `dimensions()` with default settings.
 *
 * @param path Path to a directory containing an object.
 * @return The object's dimensions.
 */
inline std::vector<size_t> dimensions(const std::filesystem::path& path) {
    State state;
    return dimensions(path, Options(), state);
}

}

#endif
