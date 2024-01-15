#ifndef TAKANE_UTILS_PUBLIC_HPP
#define TAKANE_UTILS_PUBLIC_HPP

#include <string>
#include <filesystem>
#include <unordered_map>

#include "H5Cpp.h"

#include "millijson/millijson.hpp"
#include "byteme/byteme.hpp"
#include "utils_json.hpp"

/**
 * @file utils_public.hpp
 * @brief Exported utilities.
 */

namespace takane {

/**
 * @brief Object metadata, including the type and other fields.
 */
struct ObjectMetadata {
    /**
     * Type of the object.
     */
    std::string type;
    
    /**
     * Other fields, depending on the object type.
     */
    std::unordered_map<std::string, std::shared_ptr<millijson::Base> > other;
};

/**
 * Parses a JSON object to obtain the object metadata.
 *
 * @param raw Raw JSON object, typically obtained by `millijson::parse_file`.
 * Note that `raw` is consumed by this function and should no longer be used by the caller.
 * @return Object metadata, including the type and other fields.
 */
inline ObjectMetadata reformat_object_metadata(millijson::Base* raw) {
    if (raw->type() != millijson::OBJECT) {
        throw std::runtime_error("metadata should be a JSON object");
    }

    ObjectMetadata output;
    output.other = std::move(reinterpret_cast<millijson::Object*>(raw)->values);

    auto tIt = output.other.find("type");
    if (tIt == output.other.end()) {
        throw std::runtime_error("metadata should have a 'type' property");
    }

    const auto& tval = tIt->second;
    if (tval->type() != millijson::STRING) {
        throw std::runtime_error("metadata should have a 'type' string");
    }

    output.type = std::move(reinterpret_cast<millijson::String*>(tval.get())->value);
    output.other.erase(tIt);
    return output;
}

/**
 * Reads the `OBJECT` file inside a directory to determine the object type.
 *
 * @param path Path to a directory containing an object.
 * @return Object metadata, including the type and other fields.
 */
inline ObjectMetadata read_object_metadata(const std::filesystem::path& path) try {
    std::shared_ptr<millijson::Base> obj = internal_json::parse_file(path / "OBJECT");
    return reformat_object_metadata(obj.get());
} catch (std::exception& e) {
    throw std::runtime_error("failed to read the OBJECT file at '" + path.string() + "'; " + std::string(e.what()));
}

/**
 * @brief Validation options, mostly related to input performance.
 */
struct Options {
    /**
     * Whether to parallelize reading from disk and parsing, when available.
     */
    bool parallel_reads = true;
    
    /**
     * Buffer size to use when reading data from a HDF5 file.
     */
    hsize_t hdf5_buffer_size = 10000;
};

/**
 * @cond
 */
struct State;
/**
 * @endcond
 */

/**
 * Class to map object types to `validate()` functions.
 */
typedef std::unordered_map<std::string, std::function<void(const std::filesystem::path&, const ObjectMetadata&, const Options&, State&)> > ValidateRegistry;

/**
 * Class to map object types to `dimensions()` functions.
 */
typedef std::unordered_map<std::string, std::function<std::vector<size_t>(const std::filesystem::path&, const ObjectMetadata&, const Options&, State&)> > DimensionsRegistry;

/**
 * Class to map object types to `height()` functions.
 */
typedef std::unordered_map<std::string, std::function<size_t(const std::filesystem::path&, const ObjectMetadata& m, const Options&, State&)> > HeightRegistry;

struct State {
    /**
     * Custom registry of functions to be used by `validate()`.
     * If a type is specified here, the custom function replaces the default.
     */
    ValidateRegistry validate_registry;

    /**
     * Custom registry of functions to be used by `dimensions()`.
     * If a type is specified here, the custom function replaces the default. 
     */
    DimensionsRegistry dimensions_registry;

    /**
     * Custom registry of functions to be used by `dimensions()`.
     * If a type is specified here, the custom function replaces the default. 
     */
    HeightRegistry height_registry;

    /**
     * Custom registry of derived object types and their base types, to be used by `derived_from()`.
     * Each key is the base object type and each value is the set of its derived types.
     * If a type is specified here, the set of derived types is added to the the default set.
     */
    std::unordered_map<std::string, std::unordered_set<std::string> > derived_from_registry;

    /**
     * Custom registry of object types that satisfy a particular object interface.
     * Each key is the interface and each value is the set of all types that satisfy it.
     * If a type is specified here, its set of types is added to the the default set.
     */
    std::unordered_map<std::string, std::unordered_set<std::string> > satisfies_interface_registry;
};

}

#endif
