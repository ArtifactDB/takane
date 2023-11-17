#ifndef TAKANE_UTILS_PUBLIC_HPP
#define TAKANE_UTILS_PUBLIC_HPP

#include <string>

#include "byteme/byteme.hpp"

/**
 * @file utils_public.hpp
 * @brief Exported utilities.
 */

namespace takane {

/**
 * Reads the `OBJECT` file inside a directory to determine the object type.
 *
 * @param path Path to a directory containing an object.
 * @return String containing the object type.
 */
inline std::string read_object_type(const std::string& path) {
    auto full = path + "/OBJECT";
    byteme::RawFileReader reader(full.c_str());
    std::string output;
    while (reader.load()) {
        auto buffer = reinterpret_cast<const char*>(reader.buffer());
        size_t available = reader.available();
        output.insert(output.end(), buffer, buffer + available);
    }
    return output;
}

}

#endif
