#ifndef TAKANE_DELAYED_ARRAY_HPP
#define TAKANE_DELAYED_ARRAY_HPP

#include "H5Cpp.h"
#include "ritsuko/ritsuko.hpp"
#include "chihaya/chihaya.hpp"

#include "utils_public.hpp"
#include "utils_other.hpp"

#include <vector>
#include <string>
#include <stdexcept>
#include <filesystem>
#include <cstdint>
#include <cstddef>

/**
 * @file delayed_array.hpp
 * @brief Validation for delayed arrays.
 */

namespace takane {

/**
 * @cond
 */
void validate(const std::filesystem::path&, const ObjectMetadata&, const Options&);
std::vector<std::size_t> dimensions(const std::filesystem::path&, const ObjectMetadata&, const Options&);
/**
 * @endcond
 */

/**
 * @param path Path to the directory containing a delayed array.
 * @param metadata Metadata for the object, typically read from its `OBJECT` file.
 * @param options Validation options.
 */
inline void validate_delayed_array(const std::filesystem::path& path, const ObjectMetadata& metadata, const Options& options) {
    const std::string type_name = "delayed_array"; // use a separate variable to avoid dangling reference warnings from GCC.

    try {
        const auto& type_meta = extract_json_object(metadata.other, type_name);
        try {
            const auto& vstring = extract_json_version_string(type_meta);
            auto version = ritsuko::parse_version_string(vstring.c_str(), vstring.size(), /* skip_patch = */ true);
            if (version.major != 1) {
                throw std::runtime_error("unsupported version '" + vstring + "'");
            }
        } catch (...) {
            std::throw_with_nested(std::runtime_error("failed to validate 'version'"));
        }
    } catch (...) {
        std::throw_with_nested(std::runtime_error("failed to validate '" + type_name + "' in the object metadata"));
    }

    auto chihaya_options = options.delayed_array_options;
    chihaya_options.details_only = false;

    std::uint64_t max = 0;
    std::string custom_name = "custom takane seed array";
    if (chihaya_options.array_validate_registry.find(custom_name) == chihaya_options.array_validate_registry.end()) {
        chihaya_options.array_validate_registry[custom_name] = [&](
            const H5::Group& handle,
            const ritsuko::Version& version,
            const chihaya::Options& ch_options
        ) -> chihaya::ArrayDetails {
            auto details = chihaya::validate_custom_array(handle, version, ch_options);

            std::uint64_t index;
            try {
                auto dhandle = handle.openDataSet("index");
                if (dhandle.getSpace().getSimpleExtentNdims() != 0) {
                    throw std::runtime_error("expected a scalar dataset");
                }
                if (ritsuko::hdf5::exceeds_integer_limit(dhandle, 64, false)) {
                    throw std::runtime_error("expected a datatype that fits into a 64-bit unsigned integer");
                }
                dhandle.read(&index, H5::PredType::NATIVE_UINT64);
            } catch (...) {
                std::throw_with_nested(std::runtime_error("failed to read 'index'")); 
            }

            auto seed_path = path / "seeds" / std::to_string(index);
            try {
                auto seed_meta = read_object_metadata(seed_path);
                ::validate(seed_path, seed_meta, options);

                auto seed_dims = dimensions(seed_path, seed_meta, options);
                if (seed_dims.size() != details.dimensions.size()) {
                    throw std::runtime_error("dimensionality is not consistent with 'dimensions'");
                }

                const auto ndims = seed_dims.size();
                for (I<decltype(ndims)> d = 0; d < ndims; ++d) {
                    if (seed_dims[d] != details.dimensions[d]) {
                        throw std::runtime_error("dimension extents are not consistent with 'dimensions'");
                    }
                }
            } catch (...) {
                std::throw_with_nested(std::runtime_error("failed to validate seed " + std::to_string(index)));
            }

            if (index >= max) {
                max = index + 1;
            }
            return details;
        };
    }

    try {
        H5::H5File fhandle(path / "array.h5", H5F_ACC_RDONLY);
        auto ghandle = fhandle.openGroup("delayed_array");
        ritsuko::Version chihaya_version = chihaya::extract_version(ghandle);
        if (chihaya_version.lt(1, 1, 0)) {
            throw std::runtime_error("version of the chihaya specification should be no less than 1.1");
        }
        chihaya::validate(ghandle, chihaya_version, chihaya_options);
    } catch (...) {
        std::throw_with_nested(std::runtime_error("failed to validate 'delayed_array' in 'array.h5'"));
    }

    std::size_t found = 0;
    auto seed_path = path / "seeds";
    if (std::filesystem::exists(seed_path)) {
        found = count_directory_entries(seed_path);
    }
    if (max != found) {
        throw std::runtime_error("number of objects in 'seeds' is not consistent with the number of 'index' references in 'array.h5'");
    }
}

/**
 * @param path Path to the directory containing a delayed array.
 * @param metadata Metadata for the object, typically read from its `OBJECT` file.
 * @param options Validation options.
 * @return Extent of the first dimension.
 */
inline size_t height_of_delayed_array(const std::filesystem::path& path, [[maybe_unused]] const ObjectMetadata& metadata, const Options& options) {
    auto chihaya_options = options.delayed_array_options;
    chihaya_options.details_only = true;

    H5::H5File fhandle(path / "array.h5", H5F_ACC_RDONLY);
    auto ghandle = fhandle.openGroup("delayed_array");
    auto output = chihaya::validate(ghandle, chihaya_options);
    return output.dimensions[0];
}

/**
 * @param path Path to the directory containing a delayed array.
 * @param metadata Metadata for the object, typically read from its `OBJECT` file.
 * @param options Validation options.
 * @return Dimensions of the array.
 */
inline std::vector<std::size_t> dimensions_of_delayed_array(const std::filesystem::path& path, [[maybe_unused]] const ObjectMetadata& metadata, const Options& options) {
    auto chihaya_options = options.delayed_array_options;
    chihaya_options.details_only = true;

    H5::H5File fhandle(path / "array.h5", H5F_ACC_RDONLY);
    auto ghandle = fhandle.openGroup("delayed_array");
    auto output = chihaya::validate(ghandle, chihaya_options);
    return output.dimensions;
}

}

#endif
