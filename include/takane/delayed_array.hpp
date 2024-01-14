#ifndef TAKANE_DELAYED_ARRAY_HPP
#define TAKANE_DELAYED_ARRAY_HPP

#include "ritsuko/hdf5/hdf5.hpp"
#include "ritsuko/ritsuko.hpp"
#include "chihaya/chihaya.hpp"

#include "utils_public.hpp"
#include "utils_other.hpp"

#include <vector>
#include <string>
#include <stdexcept>
#include <filesystem>
#include <cstdint>

/**
 * @file delayed_array.hpp
 * @brief Validation for delayed arrays.
 */

namespace takane {

/**
 * @cond
 */
void validate(const std::filesystem::path&, const ObjectMetadata&, const Options&);
bool derived_from(const std::string&, const std::string&);
std::vector<size_t> dimensions(const std::filesystem::path&, const ObjectMetadata&, const Options&);

namespace internal {

inline chihaya::ArrayType translate_type_to_chihaya(const std::string& type) {
    if (type == "integer") {
        return chihaya::INTEGER;
    } else if (type == "boolean") {
        return chihaya::BOOLEAN;
    } else if (type == "number") {
        return chihaya::FLOAT;
    } else if (type != "string") {
        throw std::runtime_error("cannot translate type '" + type + "' to a chihaya type");
    }
    return chihaya::STRING;
}

}
/**
 * @endcond
 */

/**
 * @namespace takane::delayed_array
 * @brief Definitions for delayed arrays.
 */
namespace delayed_array {

/**
 * @param path Path to the directory containing a delayed array.
 * @param metadata Metadata for the object, typically read from its `OBJECT` file.
 * @param options Validation options, mostly related to reading performance.
 */
inline void validate(const std::filesystem::path& path, const ObjectMetadata& metadata, const Options& options) {
    auto vstring = internal_json::extract_version_for_type(metadata.other, "delayed_array");
    auto version = ritsuko::parse_version_string(vstring.c_str(), vstring.size(), /* skip_patch = */ true);
    if (version.major != 1) {
        throw std::runtime_error("unsupported version '" + vstring + "'");
    }

    chihaya::State state;
    uint64_t max = 0;
    state.array_validate_registry["custom takane seed array"] = [&](const H5::Group& handle, const ritsuko::Version& version) -> chihaya::ArrayDetails {
        auto details = chihaya::custom_array::validate(handle, version);

        auto dhandle = ritsuko::hdf5::open_dataset(handle, "index");
        if (ritsuko::hdf5::exceeds_integer_limit(dhandle, 64, false)) {
            throw std::runtime_error("'index' should have a datatype that fits into a 64-bit unsigned integer");
        }

        auto index = ritsuko::hdf5::load_scalar_numeric_dataset<uint64_t>(dhandle);
        auto seed_path = path / "seeds" / std::to_string(index);
        auto seed_meta = read_object_metadata(seed_path);
        ::takane::validate(seed_path, seed_meta, options);

        auto seed_dims = ::takane::dimensions(seed_path, seed_meta, options);
        if (seed_dims.size() != details.dimensions.size()) {
            throw std::runtime_error("dimensionality of 'seeds/" + std::to_string(index) + "' is not consistent with 'dimensions'");
        }

        for (size_t d = 0, ndims = seed_dims.size(); d < ndims; ++d) {
            if (seed_dims[d] != details.dimensions[d]) {
                throw std::runtime_error("dimension extents of 'seeds/" + std::to_string(index) + "' is not consistent with 'dimensions'");
            }
        }

        // Peeking at the object type.
        bool found = true;
        chihaya::ArrayType expected_type;
        if (::takane::derived_from(seed_meta.type, "dense_array")) {
            auto handle = ritsuko::hdf5::open_file(seed_path / "array.h5");
            auto ghandle = ritsuko::hdf5::open_group(handle, "dense_array");
            auto dhandle = ritsuko::hdf5::open_dataset(ghandle, "data");
            auto type = ritsuko::hdf5::open_and_load_scalar_string_attribute(ghandle, "type");
            expected_type = internal::translate_type_to_chihaya(type);
        } else if (::takane::derived_from(seed_meta.type, "compressed_sparse_matrix")) {
            auto handle = ritsuko::hdf5::open_file(seed_path / "matrix.h5");
            auto ghandle = ritsuko::hdf5::open_group(handle, "compressed_sparse_matrix");
            auto dhandle = ritsuko::hdf5::open_dataset(ghandle, "data");
            auto type = ritsuko::hdf5::open_and_load_scalar_string_attribute(ghandle, "type");
            expected_type = internal::translate_type_to_chihaya(type);
        } else {
            found = false;
        }

        if (found && expected_type != details.type) {
            throw std::runtime_error("type of 'seeds/" + std::to_string(index) + "' is not consistent with 'type'");
        }

        if (index >= max) {
            max = index + 1;
        }
        return details;
    };

    auto apath = path / "array.h5";
    chihaya::validate(apath, "delayed_array", state);

    if (max != internal_other::count_directory_entries(path / "seeds")) {
        throw std::runtime_error("number of objects in 'seeds' is not consistent with the number of 'index' references in 'array.h5'");
    }
}

/**
 * @param path Path to the directory containing a delayed array.
 * @param metadata Metadata for the object, typically read from its `OBJECT` file.
 * @param options Validation options, mostly related to reading performance.
 * @return Extent of the first dimension.
 */
inline size_t height(const std::filesystem::path& path, [[maybe_unused]] const ObjectMetadata& metadata, [[maybe_unused]] const Options& options) {
    auto apath = path / "array.h5";
    auto output = chihaya::validate(apath, "delayed_array");
    return output.dimensions[0];
}

/**
 * @param path Path to the directory containing a delayed array.
 * @param metadata Metadata for the object, typically read from its `OBJECT` file.
 * @param options Validation options, mostly related to reading performance.
 * @return Dimensions of the array.
 */
inline std::vector<size_t> dimensions(const std::filesystem::path& path, [[maybe_unused]] const ObjectMetadata& metadata, [[maybe_unused]] const Options& options) {
    auto apath = path / "array.h5";
    auto output = chihaya::validate(apath, "delayed_array");
    return std::vector<size_t>(output.dimensions.begin(), output.dimensions.end());
}

}

}

#endif
