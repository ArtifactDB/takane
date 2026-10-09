#ifndef TAKANE_UTILS_SUMMARIZED_EXPERIMENT_HPP
#define TAKANE_UTILS_SUMMARIZED_EXPERIMENT_HPP

#include "millijson/millijson.hpp"
#include "sanisizer/sanisizer.hpp"

#include "utils_json.hpp"
#include "utils_other.hpp"

#include <unordered_set>
#include <string>
#include <stdexcept>
#include <filesystem>
#include <cmath>
#include <cstddef>

namespace takane {

inline std::pair<std::size_t, std::size_t> extract_summarized_experiment_dimensions(const JsonObjectMap& semap) {
    std::size_t num_rows = 0, num_cols = 0;

    auto dIt = semap.find("dimensions");
    if (dIt == semap.end()) {
        throw std::runtime_error("expected a 'dimensions' property");
    }
    const auto& dims = dIt->second;
    if (dims->type() != millijson::ARRAY) {
        throw std::runtime_error("expected 'dimensions' to be an array");
    }

    auto dptr = reinterpret_cast<const millijson::Array*>(dims.get());
    if (dptr->value().size() != 2) {
        throw std::runtime_error("expected 'dimensions' to be an array of length 2");
    }

    std::size_t counter = 0;
    for (const auto& x : dptr->value()) {
        if (x->type() != millijson::NUMBER) {
            throw std::runtime_error("expected 'dimensions' to be an array of numbers");
        }

        const auto raw_val = reinterpret_cast<const millijson::Number*>(x.get())->value();
        if (raw_val < 0 || std::floor(raw_val) != raw_val) {
            throw std::runtime_error("expected 'dimensions' to contain non-negative integers");
        }

        const auto val = sanisizer::from_float<std::size_t>(raw_val);           
        if (counter == 0) {
            num_rows = val;
        } else {
            num_cols = val;
        }

        ++counter;
    }

    return std::make_pair(num_rows, num_cols);
}

inline std::vector<std::string> extract_summarized_experiment_names(const std::filesystem::path& path) {
    auto parsed = parse_json_file(path);
    if (parsed->type() != millijson::ARRAY) {
        throw std::runtime_error("expected an array");
    }

    auto aptr = reinterpret_cast<const millijson::Array*>(parsed.get());
    const auto number = aptr->value().size();

    std::vector<std::string> contents;
    contents.reserve(number);
    std::unordered_set<std::string> present;
    present.reserve(number);

    for (I<decltype(number)> i = 0; i < number; ++i) {
        auto eptr = aptr->value()[i];
        if (eptr->type() != millijson::STRING) {
            throw std::runtime_error("expected an array of strings");
        }

        auto nptr = reinterpret_cast<const millijson::String*>(eptr.get());
        auto name = nptr->value();
        if (name.empty()) {
            throw std::runtime_error("name should not be an empty string");
        }
        if (present.find(name) != present.end()) {
            throw std::runtime_error("detected duplicated name '" + name + "'");
        }
        contents.push_back(name);
        present.insert(std::move(name));
    }

    return contents;
}

}

#endif
