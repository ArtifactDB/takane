#ifndef TAKANE_HEIGHT_HPP
#define TAKANE_HEIGHT_HPP

#include <functional>
#include <string>
#include <stdexcept>
#include <filesystem>

#include "utils_public.hpp"
#include "atomic_vector.hpp"
#include "string_factor.hpp"

/**
 * @file HEIGHT.hpp
 * @brief Dispatch to functions for the object's height.
 */

namespace takane {

/**
 * @cond
 */
namespace internal_HEIGHT {

inline auto default_registry() {
    std::unordered_map<std::string, std::function<size_t(const std::filesystem::path&, const Options&)> > registry;
    registry["data_frame"] = [](const std::filesystem::path& p, const Options& o) -> size_t { return data_frame::height(p, o); };
    return registry;
} 

}
/**
 * @endcond
 */

/**
 * Registry of functions to be used by `HEIGHT()`.
 */
inline std::unordered_map<std::string, std::function<size_t(const std::filesystem::path&, const Options&)> > HEIGHT_registry = internal_HEIGHT::default_registry();

/**
 * Get the height of an object in a subdirectory, based on the supplied object type.
 * This is used to check the shape of objects stored in vertical containers, e.g., columns of a `data_frame`.
 * For vectors or other 1-dimensional objects, the height is usually just the length;
 * for higher dimensional objects, the height is usually the extent of the first dimension.
 *
 * @param path Path to a directory representing an object.
 * @param type Type of the object, typically determined from its `OBJECT` file.
 * @param options Validation options, mostly for input performance.
 *
 * @return The object's height.
 */
inline size_t HEIGHT(const std::filesystem::path& path, const std::string& type, const Options& options) {
    if (!std::filesystem::exists(path) || std::filesystem::status(path).type() != std::filesystem::file_type::directory) {
        throw std::runtime_error("expected '" + path.string() + "' to be a directory");
    }

    auto vrIt = HEIGHT_registry.find(type);
    if (vrIt == HEIGHT_registry.end()) {
        throw std::runtime_error("failed to find a HEIGHT function for object type '" + type + "' at '" + path.string() + "'");
    }

    return (vrIt->second)(path, options);
}

/**
 * Get the height of an object in a subdirectory, using its `OBJECT` file to automatically determine the type.
 *
 * @param path Path to a directory containing an object.
 * @param options Validation options, mostly for input performance.
 * @return The object's height.
 */
inline size_t HEIGHT(const std::filesystem::path& path, const Options& options) {
    return HEIGHT(path, read_object_type(path), options);
}

/**
 * Overload of `HEIGHT()` with default options.
 *
 * @param path Path to a directory containing an object.
 * @return The object's height.
 */
inline size_t HEIGHT(const std::filesystem::path& path) {
    return HEIGHT(path, Options());
}

}

#endif
