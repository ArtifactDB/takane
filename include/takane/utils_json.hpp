#ifndef TAKANE_UTILS_JSON_HPP
#define TAKANE_UTILS_JSON_HPP

#include <string>
#include <stdexcept>
#include <unordered_map>

#include "millijson/millijson.hpp"

namespace takane {

namespace internal_json {

typedef std::unordered_map<std::string, std::shared_ptr<millijson::Base> > JsonObjectMap;

inline const JsonObjectMap& extract_object(const JsonObjectMap& x, const std::string& name) {
    auto xIt = x.find(name);
    if (xIt == x.end()) {
        throw std::runtime_error("property is not present");
    }
    const auto& val = xIt->second;
    if (val->type() != millijson::OBJECT) {
        throw std::runtime_error("property should be a JSON object");
    }
    return reinterpret_cast<millijson::Object*>(val.get())->values;
}

inline const std::string& extract_string(const JsonObjectMap& x, const std::string& name) {
    auto xIt = x.find(name);
    if (xIt == x.end()) {
        throw std::runtime_error("property is not present");
    }
    const auto& val = xIt->second;
    if (val->type() != millijson::STRING) {
        throw std::runtime_error("property should be a JSON string");
    }
    return reinterpret_cast<millijson::String*>(val.get())->value;
}

inline const std::string& extract_version_string(const JsonObjectMap& x, const std::string& type) try {
    return extract_string(extract_object(x, type), "version");
} catch (std::exception& e) {
    throw std::runtime_error("failed to extract '" + type + ".version' from the object metadata; " + std::string(e.what()));
}

}

}

#endif
