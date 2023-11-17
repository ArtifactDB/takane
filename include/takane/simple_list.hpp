#ifndef TAKANE_SIMPLE_LIST_HPP
#define TAKANE_SIMPLE_LIST_HPP

#include "uzuki2/uzuki2.hpp"
#include "byteme/byteme.hpp"

#include <string>
#include <stdexcept>
#include <filesystem>

/**
 * @file simple_list.hpp
 * @brief Validation for simple lists.
 */

namespace takane {

/**
 * @cond
 */
void validate(const std::string&);
/**
 * @endcond
 */

/**
 * @namespace takane::simple_list
 * @brief Definitions for simple lists.
 */
namespace simple_list {

/**
 * @brief Parameters for validating the simple list file.
 */
struct Parameters {
    /**
     * Whether to read and parse the JSON in parallel.
     */
    bool parallel = true;
};

/**
 * @param path Path to the directory containing the simple list.
 * @param params Validation parameters.
 */
inline void validate(const std::string& path, Parameters params = Parameters()) try {
    std::string other_dir = path + "/other_contents";
    int num_external = 0;
    if (std::filesystem::exists(other_dir)) {
        auto status = std::filesystem::status(other_dir);
        if (status.type() != std::filesystem::file_type::directory) {
            throw std::runtime_error("expected 'other_contents' to be a directory");
        } 

        for (const auto& entry : std::filesystem::directory_iterator(other_dir)) {
            try {
                ::takane::validate(entry.path().string());
            } catch (std::exception& e) {
                throw std::runtime_error("failed to validate external list object at '" + std::filesystem::relative(entry.path(), path).string() + "'; " + std::string(e.what()));
            }
            ++num_external;
        }
    }

    auto candidate = path + "/list_contents.json.gz";
    if (std::filesystem::exists(candidate)) {
        uzuki2::json::Options opt;
        opt.parallel = params.parallel;
        byteme::SomeFileReader gzreader(candidate);
        uzuki2::json::validate(gzreader, num_external, opt);
    } else {
        throw std::runtime_error("could not determine format from the file names");
    }

} catch (std::exception& e) {
    throw std::runtime_error("failed to validate a 'simple_list' at '" + path + "'; " + std::string(e.what()));
}

}

}

#endif
