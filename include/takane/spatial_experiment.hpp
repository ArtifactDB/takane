#ifndef TAKANE_SPATIAL_EXPERIMENT_HPP
#define TAKANE_SPATIAL_EXPERIMENT_HPP

#include "ritsuko/hdf5/hdf5.hpp"

#include "single_cell_experiment.hpp"
#include "utils_factor.hpp"
#include "utils_public.hpp"
#include "utils_other.hpp"

#include <filesystem>
#include <stdexcept>
#include <unordered_set>
#include <string>
#include <vector>
#include <cmath>

namespace takane {

/**
 * @namespace takane::spatial_experiment
 * @brief Definitions for spatial experiments.
 */
namespace spatial_experiment {

/**
 * @cond
 */
namespace internal {

inline void validate_coordinates(const std::filesystem::path& path, size_t ncols, const Options& options) {
    auto coord_path = path / "coordinates";
    auto coord_meta = read_object_metadata(coord_path);
    if (coord_meta.type != "dense_array") {
        throw std::runtime_error("'spatial_coordinates' should be a dense array");
    }

    // Validating the coordinates; currently these must be a dense array of
    // points, but could also be polygons/hulls in the future.
    try {
        ::takane::validate(coord_path, coord_meta, options);
    } catch (std::exception& e) {
        throw std::runtime_error("failed to validate 'spatial_coordinates'; " + std::string(e.what()));
    }

    auto cdims = ::takane::dimensions(coord_path, coord_meta, options);
    if (cdims.size() != 2) {
        throw std::runtime_error("'spatial_coordinates' should be a 2-dimensional dense array");
    } else if (cdims[1] != 2 && cdims[1] != 3) {
        throw std::runtime_error("'spatial_coordinates' should have 2 or 3 columns");
    } else if (cdims[0] != ncols) {
        throw std::runtime_error("number of rows in 'spatial_coordinates' should equal the number of columns in the 'spatial_experiment'");
    }

    // Checking that the values are numeric.
    auto handle = ritsuko::hdf5::open_file(coord_path / "array.h5");
    auto ghandle = ritsuko::hdf5::open_group(handle, "dense_array");
    auto dhandle = ritsuko::hdf5::open_dataset(ghandle, "data");
    auto dclass = dhandle.getTypeClass();
    if (dclass != H5T_INTEGER && dclass != H5T_FLOAT) {
        throw std::runtime_error("values in 'spatial_coordinates' should be numeric");
    }
}

inline void validate_images(const std::filesystem::path& path, size_t ncols, const Options& options) {
    auto image_dir = path / "images";
    auto mappath = image_dir / "mapping.h5";
    auto ihandle = ritsuko::hdf5::open_file(mappath);
    auto ghandle = ritsuko::hdf5::open_group(ihandle, "spatial_experiment");

    size_t num_images = 0;
    try {
        struct SampleMapMessenger {
            static std::string level() { return "sample name"; }
            static std::string levels() { return "sample names"; }
            static std::string codes() { return "sample assignments"; }
        };

        auto num_samples = internal_factor::validate_factor_levels<SampleMapMessenger>(ghandle, "sample_names", options.hdf5_buffer_size);
        auto num_codes = internal_factor::validate_factor_codes<SampleMapMessenger>(ghandle, "column_samples", num_samples, options.hdf5_buffer_size, true);
        if (num_codes != ncols) {
            throw std::runtime_error("length of 'column_samples' should equal the number of columns in the spatial experiment");
        }

        // Scanning through the image information.
        size_t num_images = 0;
        {
            auto ishandle = ritsuko::hdf5::open_dataset(ghandle, "image_samples");
            if (ritsuko::hdf5::exceeds_integer_limit(ishandle, 64, false)) {
                throw std::runtime_error("expected a datatype for 'image_samples' that fits in a 64-bit unsigned integer");
            }
            num_images = ritsuko::hdf5::get_1d_length(ishandle.getSpace(), false);

            auto iihandle = ritsuko::hdf5::open_dataset(ghandle, "image_ids");
            if (iihandle.getTypeClass() != H5T_STRING) {
                throw std::runtime_error("expected a string datatype for 'image_ids'");
            }
            if (ritsuko::hdf5::get_1d_length(iihandle.getSpace(), false) != num_images) {
                throw std::runtime_error("expected 'image_ids' to have the same length as 'image_samples'");
            }

            ritsuko::hdf5::Stream1dNumericDataset<uint64_t> isstream(&ishandle, num_images, options.hdf5_buffer_size);
            ritsuko::hdf5::Stream1dStringDataset iistream(&iihandle, num_images, options.hdf5_buffer_size);
            std::vector<std::unordered_set<std::string> > collected(num_samples);

            for (hsize_t i = 0; i < num_images; ++i, isstream.next(), iistream.next()) {
                auto id = isstream.get();
                if (id >= num_samples) {
                    throw std::runtime_error("entries of 'image_samples' should be less than the number of samples");
                }

                auto& present = collected[id];
                auto x = iistream.steal();
                if (present.find(x) != present.end()) {
                    throw std::runtime_error("'image_ids' contains duplicated image IDs for the same sample + ('" + x + "')");
                }
                present.insert(std::move(x));
            }

            for (const auto& x : collected) {
                if (x.empty()) {
                    throw std::runtime_error("each sample should map to one or more images in 'image_samples'");
                }
            }
        }

        {
            auto sihandle = ritsuko::hdf5::open_dataset(ghandle, "image_scale_factors");
            if (ritsuko::hdf5::exceeds_float_limit(sihandle, 64)) {
                throw std::runtime_error("expected a datatype for 'image_scale_factors' that fits in a 64-bit float");
            }
            if (ritsuko::hdf5::get_1d_length(sihandle.getSpace(), false) != num_images) {
                throw std::runtime_error("expected 'image_scale_factors' to have the same length as 'image_samples'");
            }

            ritsuko::hdf5::Stream1dNumericDataset<double> sistream(&sihandle, num_images, options.hdf5_buffer_size);
            for (hsize_t i = 0; i < num_images; ++i, sistream.next()) {
                auto x = sistream.get();
                if (!std::isfinite(x) || x <= 0) {
                    throw std::runtime_error("entries of 'image_scale_factors' should be finite and positive");
                }
            }
        }
    } catch (std::exception& e) {
        throw std::runtime_error("failed to validate '" + mappath.string() + "'; " + std::string(e.what()));
    }

    // Now validating the images themselves.
    for (size_t i = 0; i < num_images; ++i) {
        auto image_name = std::to_string(i);
        auto image_path = image_dir / image_name;
        auto image_meta = read_object_metadata(image_path);
        if (image_meta.type != "spatial_image") {
            throw std::runtime_error("expected a 'spatial_image' object at '" + image_path.string() + "'");
        }
        try {
            ::takane::validate(image_path, image_meta, options);
        } catch (std::exception& e) {
            throw std::runtime_error("failed to validate image at '" + image_path.string() + "'; " + std::string(e.what()));
        }
    }

    size_t num_dir_obj = internal_other::count_directory_entries(image_dir);
    if (num_dir_obj - 1 != num_images) { // -1 to account for the mapping.h5 file itself.
        throw std::runtime_error("more objects than expected inside the 'images' subdirectory");
    }
}

}
/**
 * @endcond
 */

/**
 * @param path Path to the directory containing the spatial experiment.
 * @param metadata Metadata for the object, typically read from its `OBJECT` file.
 * @param options Validation options, typically for reading performance.
 */
inline void validate(const std::filesystem::path& path, const ObjectMetadata& metadata, const Options& options) {
    ::takane::single_cell_experiment::validate(path, metadata, options);
    auto dims = ::takane::summarized_experiment::dimensions(path, metadata, options);
    internal::validate_coordinates(path, dims[1], options);
    internal::validate_images(path, dims[1], options);
}

}

}

#endif
