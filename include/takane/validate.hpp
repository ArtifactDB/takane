#ifndef TAKANE_VALIDATE_HPP
#define TAKANE_VALIDATE_HPP

#include <functional>
#include <string>

#include "utils_public.hpp"
#include "atomic_vector.hpp"
#include "string_factor.hpp"

/**
 * @file validate.hpp
 * @brief Validation dispatch function.
 */

namespace takane {

/**
 * @cond
 */
namespace internal_validate {

inline auto default_registry() {
    std::unordered_map<std::string, std::function<void(const std::string&)> > registry;
    registry["atomic_vector"] = [](const std::string& path) { atomic_vector::validate(path); };
    registry["string_factor"] = [](const std::string& path) { string_factor::validate(path); };
    return registry;
} 

}
/**
 * @endcond
 */

/**
 * Registry of validation functions, to be used by `validate()`.
 */
inline std::unordered_map<std::string, std::function<void(const std::string&)> > validation_registry = internal_validate::default_registry();

/**
 * Validate an object in a subdirectory, based on the supplied object type.
 *
 * @param path Path to a directory containing an object.
 * @param type Type of the object, typically determined from its `OBJECT` file.
 */
inline void validate(const std::string& path, const std::string& type) {
    auto vrIt = validation_registry.find(type);
    if (vrIt != validation_registry.end()) {
        (vrIt->second)(path);
    } else {
        throw std::runtime_error("failed to find a validation function for object type '" + type + "' at '" + path + "'");
    }
}

/**
 * Validate an object in a subdirectory, using its `OBJECT` file to determine the type.
 *
 * @param path Path to a directory containing an object.
 */
inline void validate(const std::string& path) {
    validate(path, read_object_type(path));
}

}

#endif
