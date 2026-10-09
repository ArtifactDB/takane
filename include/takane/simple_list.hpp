#ifndef TAKANE_SIMPLE_LIST_HPP
#define TAKANE_SIMPLE_LIST_HPP

#include <string>
#include <stdexcept>
#include <filesystem>
#include <cstddef>
#include <optional>
#include <cmath>

#include "H5Cpp.h"
#include "uzuki2/uzuki2.hpp"
#include "byteme/byteme.hpp"

#include "utils_public.hpp"
#include "utils_other.hpp"

/**
 * @file simple_list.hpp
 * @brief Validation for simple lists.
 */

namespace takane {

/**
 * @cond
 */
void validate(const std::filesystem::path&, const Options&);

inline std::string extract_simple_list_format(const JsonObjectMap& map) {
    auto fIt = map.find("format");
    if (fIt == map.end()) {
        return "hdf5";
    }
    const auto& val = fIt->second;
    if (val->type() != millijson::STRING) {
        throw std::runtime_error("expected a JSON string");
    }
    return reinterpret_cast<millijson::String*>(val.get())->value();
}

inline std::optional<std::size_t> extract_simple_list_length(const JsonObjectMap& map) {
    std::optional<std::size_t> output;
    auto lIt = map.find("length");
    if (lIt != map.end()) {
        const auto& val = lIt->second;
        if (val->type() != millijson::NUMBER) {
            throw std::runtime_error("expected a JSON number");
        }
        const auto num = reinterpret_cast<millijson::Number*>(val.get())->value();
        if (num != std::trunc(num)) {
            throw std::runtime_error("expected an integer");
        }
        output = sanisizer::from_float<std::size_t>(num);
    }
    return output;
}
/**
 * @endcond
 */

/**
 * @param path Path to the directory containing the simple list.
 * @param metadata Metadata for the object, typically read from its `OBJECT` file.
 * @param options Validation options.
 */
inline void validate_simple_list(const std::filesystem::path& path, const ObjectMetadata& metadata, const Options& options) {
    const std::string type_name = "simple_list"; // use a separate variable to avoid dangling reference warnings from GCC.

    ritsuko::Version version;
    std::string format;
    std::optional<std::size_t> expected_length;
    try {
        const auto& metamap = extract_json_object(metadata.other, type_name);
        try {
            const std::string& vstring = extract_json_version_string(metamap);
            version = ritsuko::parse_version_string(vstring.c_str(), vstring.size(), /* skip_patch = */ true);
            if (version.major != 1) {
                throw std::runtime_error("unsupported version string '" + vstring + "'");
            }
        } catch (...) {
            std::throw_with_nested(std::runtime_error("failed to validate 'version'"));
        }
        
        try {
            format = extract_simple_list_format(metamap);
            if (format != "json.gz" && format != "hdf5") {
                throw std::runtime_error("unknown format '" + format + "'");
            }
        } catch (...) {
            std::throw_with_nested(std::runtime_error("failed to validate 'format'"));
        }

        try {
            expected_length = extract_simple_list_length(metamap);
        } catch (...) {
            std::throw_with_nested(std::runtime_error("failed to validate 'length'"));
        }
    } catch (...) {
        std::throw_with_nested(std::runtime_error("failed to validate '" + type_name + "' in the object metadata"));
    }

    auto other_dir = path / "other_contents";
    std::size_t num_external = 0;
    if (std::filesystem::exists(other_dir)) {
        auto status = std::filesystem::status(other_dir);
        if (status.type() != std::filesystem::file_type::directory) {
            throw std::runtime_error("expected 'other_contents' to be a directory");
        } 

        num_external = count_directory_entries(other_dir);
        for (I<decltype(num_external)> e = 0; e < num_external; ++e) {
            auto epath = other_dir / std::to_string(e);
            if (!std::filesystem::exists(epath)) {
                throw std::runtime_error("expected an external list object at '" + std::filesystem::relative(epath, path).string() + "'");
            }
            try {
                validate(epath, options);
            } catch (...) {
                std::throw_with_nested(std::runtime_error("failed to validate external list object at '" + std::filesystem::relative(epath, path).string() + "'"));
            }
        }
    }

    I<decltype(std::declval<uzuki2::List>().size())> len;
    if (format == "json.gz") {
        try {
            uzuki2::json::Options opt;
            opt.parallel = options.parallel_reads;
            auto gzreader = open_reader<byteme::GzipFileReader>(path / "list_contents.json.gz", byteme::GzipFileReaderOptions());
            auto loaded = uzuki2::json::parse<uzuki2::DummyProvisioner>(*gzreader, uzuki2::DummyExternals(num_external), opt);
            len = reinterpret_cast<const uzuki2::List*>(loaded.get())->size();
        } catch (...) {
            std::throw_with_nested(std::runtime_error("failed to validate 'list_contents.json.gz'"));
        }
    } else {
        try {
            H5::H5File handle(path / "list_contents.h5", H5F_ACC_RDONLY);
            auto ghandle = handle.openGroup(type_name);
            auto loaded = uzuki2::hdf5::parse<uzuki2::DummyProvisioner>(ghandle, uzuki2::DummyExternals(num_external), {});
            len = reinterpret_cast<const uzuki2::List*>(loaded.get())->size();
        } catch (...) {
            std::throw_with_nested(std::runtime_error("failed to validate '" + type_name + "' in 'list_contents.h5'"));
        }
    }

    if (version.ge(1, 1, 0)) {
        if (expected_length.has_value() && *expected_length != len) {
            throw std::runtime_error("value of '/" + type_name + "/length' from the object metadata differs from the length of the list");
        }
    }
}

/**
 * @param path Path to the directory containing the simple list.
 * @param metadata Metadata for the object, typically read from its `OBJECT` file.
 * @param options Validation options.
 * @return The number of list elements.
 */
inline std::size_t height_of_simple_list(const std::filesystem::path& path, const ObjectMetadata& metadata, const Options& options) {
    const std::string type_name = "simple_list"; // use a separate variable to avoid dangling reference warnings from GCC.

    const auto& metamap = extract_json_object(metadata.other, type_name);
    auto len_info = extract_simple_list_length(metamap);
    if (len_info.has_value()) {
        return *len_info;
    }

    std::string format = extract_simple_list_format(metamap);
    if (format == "hdf5") {
        H5::H5File handle(path / "list_contents.h5", H5F_ACC_RDONLY);
        auto lhandle = handle.openGroup("simple_list");
        auto vhandle = lhandle.openGroup("data");
        return vhandle.getNumObjs();

    } else {
        // Not much choice but to parse the entire list here. We do so using the
        // dummy, which still has enough self-awareness to hold its own length.
        auto other_dir = path / "other_contents";
        std::size_t num_external = 0;
        if (std::filesystem::exists(other_dir)) {
            num_external = count_directory_entries(other_dir);
        }

        uzuki2::json::Options opt;
        opt.parallel = options.parallel_reads;
        auto gzreader = open_reader<byteme::GzipFileReader>(path / "list_contents.json.gz", byteme::GzipFileReaderOptions());
        auto ptr = uzuki2::json::parse<uzuki2::DummyProvisioner>(*gzreader, uzuki2::DummyExternals(num_external), opt);
        return reinterpret_cast<const uzuki2::List*>(ptr.get())->size();
    }
}

}

#endif
