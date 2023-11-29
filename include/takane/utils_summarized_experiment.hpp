#ifndef TAKANE_UTILS_SUMMARIZED_EXPERIMENT_HPP
#define TAKANE_UTILS_SUMMARIZED_EXPERIMENT_HPP

#include "millijson/millijson.hpp"

#include <unordered_set>
#include <string>
#include <stdexcept>
#include <filesystem>

namespace takane {

namespace internal_summarized_experiment {

inline size_t check_names_json(const std::filesystem::path& dir) try {
    auto npath = dir / "names.json";
    auto parsed = millijson::parse_file(npath.c_str());
    if (parsed->type() != millijson::ARRAY) {
        throw std::runtime_error("expected an array");
    }

    auto aptr = reinterpret_cast<const millijson::Array*>(parsed.get());
    size_t number = aptr->values.size();
    std::unordered_set<std::string> present;
    present.reserve(number);

    for (size_t i = 0; i < number; ++i) {
        auto eptr = aptr->values[i];
        if (eptr->type() != millijson::STRING) {
            throw std::runtime_error("expected an array of strings");
        }

        auto nptr = reinterpret_cast<const millijson::String*>(eptr.get());
        auto name = nptr->value;
        if (name.empty()) {
            throw std::runtime_error("name should not be an empty string");
        }
        if (present.find(name) != present.end()) {
            throw std::runtime_error("detected duplicated name '" + name + "'");
        }
        present.insert(std::move(name));
    }

    return number;
} catch (std::exception& e) {
    throw std::runtime_error("invalid '" + dir.string() + "/names.json' file; " + std::string(e.what()));
}

}

}

#endif
