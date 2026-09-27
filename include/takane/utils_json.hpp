#ifndef TAKANE_UTILS_JSON_HPP
#define TAKANE_UTILS_JSON_HPP

#include <string>
#include <stdexcept>
#include <unordered_map>
#include <filesystem>

#include "millijson/millijson.hpp"

namespace takane {

typedef std::unordered_map<std::string, std::shared_ptr<millijson::Base> > JsonObjectMap;

template<typename Path_>
std::shared_ptr<millijson::Base> parse_json_file(const Path_& path) {
    if constexpr(std::is_same<typename Path_::value_type, char>::value) {
        return millijson::parse_file(path.c_str(), {});
    } else {
        auto cpath = path.string();
        return millijson::parse_file(cpath.c_str(), {});
    }
}

inline const JsonObjectMap& extract_json_object(const JsonObjectMap& x, const std::string& name) {
    auto xIt = x.find(name);
    if (xIt == x.end()) {
        throw std::runtime_error("'" + name + "' property is not present");
    }
    const auto& val = xIt->second;
    if (val->type() != millijson::OBJECT) {
        throw std::runtime_error("'" + name + "' property should be a JSON object");
    }
    return reinterpret_cast<millijson::Object*>(val.get())->value();
}

inline const std::string& extract_json_string(const JsonObjectMap& x, const std::string& name) {
    auto xIt = x.find(name);
    if (xIt == x.end()) {
        throw std::runtime_error("'" + name + "' property is not present");
    }
    const auto& val = xIt->second;
    if (val->type() != millijson::STRING) {
        throw std::runtime_error("'" + name + "' property should be a JSON string");
    }
    return reinterpret_cast<millijson::String*>(val.get())->value();
}

inline const JsonObjectMap& extract_json_type_metadata(const JsonObjectMap& x, const std::string& type) {
    const JsonObjectMap* output = NULL;
    try {
        output = &(extract_json_object(x, type));
    } catch (std::exception& e) {
        throw std::runtime_error("failed to extract '" + type + "' from the object metadata; " + std::string(e.what())); 
    }
    return *output;
}

inline const std::string& extract_json_version_string(const JsonObjectMap& x, const std::string& type) {
    const std::string* output = NULL;
    const std::string version = "version";
    try {
        output = &(extract_json_string(x, version));
    } catch (std::exception& e) {
        throw std::runtime_error("failed to extract '/" + type + "/version' from the object metadata; " + std::string(e.what())); 
    }
    return *output;
}

}

#endif
