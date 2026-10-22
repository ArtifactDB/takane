#ifndef TAKANE_SPATIAL_EXPERIMENT_HPP
#define TAKANE_SPATIAL_EXPERIMENT_HPP

#include "ritsuko/ritsuko.hpp"
#include "sanisizer/sanisizer.hpp"

#include "single_cell_experiment.hpp"
#include "image_file.hpp"
#include "utils_factor.hpp"
#include "utils_public.hpp"
#include "utils_other.hpp"
#include "utils_files.hpp"

#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>
#include <cmath>

/**
 * @file spatial_experiment.hpp
 * @brief Validation for spatial experiments.
 */

namespace takane {

/**
 * @cond
 */
bool derived_from(const std::string&, const std::string&, const Options& options);
void validate(const std::filesystem::path&, const ObjectMetadata&, const Options& options);
bool satisfies_interface(const std::string&, const std::string&, const Options& options);

inline void validate_spatial_coordinates(const std::filesystem::path& coord_path, std::size_t ncols, const Options& options) {
    auto coord_meta = read_object_metadata(coord_path);
    if (!derived_from(coord_meta.type, "dense_array", options)) {
        throw std::runtime_error("expected a dense array");
    }

    // Validating the coordinates; currently these must be a dense array of
    // points, but could also be polygons/hulls in the future.
    validate(coord_path, coord_meta, options);

    auto cdims = dimensions(coord_path, coord_meta, options);
    if (cdims.size() != 2) {
        throw std::runtime_error("expected a 2-dimensional dense array");
    } else if (cdims[1] != 2 && cdims[1] != 3) {
        throw std::runtime_error("expected 2 or 3 columns");
    } else if (cdims[0] != ncols) {
        throw std::runtime_error("number of rows in should equal the number of columns in the 'spatial_experiment'");
    }

    // Checking that the values are numeric.
    H5::H5File handle(coord_path / "array.h5", H5F_ACC_RDONLY);
    auto ghandle = handle.openGroup("dense_array");
    auto dhandle = ghandle.openDataSet("data");
    auto dclass = dhandle.getTypeClass();
    if (dclass != H5T_INTEGER && dclass != H5T_FLOAT) {
        throw std::runtime_error("values should be numeric");
    }
}

inline void validate_spatial_images(const std::filesystem::path& image_dir, std::size_t ncols, const ritsuko::Version& version, const Options& options) {
    if (!std::filesystem::exists(image_dir) && version.ge(1, 2, 0)) {
        // No images at all, which is permitted.
        return;
    }

    auto mappath = image_dir / "mapping.h5";
    std::vector<std::string> image_formats;

    try {
        H5::H5File ihandle(mappath, H5F_ACC_RDONLY);
        auto ghandle = ihandle.openGroup("spatial_experiment");

        struct SampleMapMessenger {
            static std::string level() { return "sample name"; }
            static std::string levels() { return "sample names"; }
            static std::string codes() { return "sample assignments"; }
        };

        hsize_t num_samples;
        try {
            auto lhandle = ghandle.openDataSet("sample_names");
            num_samples = validate_factor_levels<SampleMapMessenger>(lhandle, options.hdf5_buffer_size);
        } catch (...) {
            std::throw_with_nested(std::runtime_error("failed to validate 'sample_names'"));
        }

        try {
            auto chandle = ghandle.openDataSet("column_samples");
            hsize_t num_codes = validate_factor_codes<SampleMapMessenger>(chandle, num_samples, options.hdf5_buffer_size, true);
            if (num_codes != ncols) {
                throw std::runtime_error("length should equal the number of columns in the spatial experiment");
            }
        } catch (...) {
            std::throw_with_nested(std::runtime_error("failed to validate 'column_samples'"));
        }

        hsize_t num_images;
        std::vector<std::pair<std::uint64_t, std::string> > sample_and_ids;
        try {
            auto sample_handle = ghandle.openDataSet("image_samples");
            if (ritsuko::hdf5::exceeds_integer_limit(sample_handle, 64, false)) {
                throw std::runtime_error("expected a datatype that fits in a 64-bit unsigned integer");
            }

            auto sample_space = sample_handle.getSpace();
            if (sample_space.getSimpleExtentNdims() != 1) {
                throw std::runtime_error("expected a 1-dimensional dataset");
            }
            sample_space.getSimpleExtentDims(&num_images);
            sanisizer::resize(sample_and_ids, num_images);

            ritsuko::hdf5::Stream1dNumericDataset<std::uint64_t> sample_stream(
                &sample_handle,
                num_images,
                [&]{
                    ritsuko::hdf5::Stream1dNumericDatasetOptions opt;
                    opt.contiguous_chunk_size = options.hdf5_buffer_size;
                    return opt;
                }()
            );

            auto present = sanisizer::create<std::vector<unsigned char> >(num_samples); 
            iterate_stream<std::uint64_t>(
                sample_stream,
                [&](hsize_t i, std::uint64_t sample) -> void {
                    if (sample >= num_samples) {
                        throw std::runtime_error("entries should be less than the number of samples");
                    }
                    sample_and_ids[i].first = sample;
                    present[sample] = 1;
                }
            );

            for (auto x : present) {
                if (!x) {
                    throw std::runtime_error("each sample should be represented by at least one image");
                }
            }
        } catch (...) {
            std::throw_with_nested(std::runtime_error("failed to validate 'image_samples'"));
        }

        try {
            auto id_handle = ghandle.openDataSet("image_ids");
            if (!ritsuko::hdf5::is_utf8_string(id_handle)) {
                throw std::runtime_error("expected 'image_ids' to have a datatype that can be represented by a UTF-8 encoded string");
            }

            auto id_space = id_handle.getSpace();
            if (id_space.getSimpleExtentNdims() != 1) {
                throw std::runtime_error("expected a 1-dimensional dataset");
            }

            hsize_t num_ids;
            id_space.getSimpleExtentDims(&num_ids);
            if (num_ids != num_images) {
                throw std::runtime_error("expected length to be the same as that of 'image_samples'");
            }

            ritsuko::hdf5::Stream1dStringDataset id_stream(
                &id_handle,
                num_images,
                [&]{
                    ritsuko::hdf5::Stream1dStringDatasetOptions opt;
                    opt.contiguous_chunk_size = options.hdf5_buffer_size;
                    return opt;
                }()
            );

            iterate_stream<std::string>(
                id_stream,
                [&](hsize_t i, std::string val) -> void {
                    sample_and_ids[i].second = std::move(val);
                }
            );

            std::sort(sample_and_ids.begin(), sample_and_ids.end());
            for (hsize_t i = 1; i < num_images; ++i) {
                const auto& current = sample_and_ids[i];
                if (current == sample_and_ids[i - 1]) {
                    throw std::runtime_error("detected duplicated image ID '" + current.second + "' for sample " + ritsuko::itoa(current.first));
                }
            }
        } catch (...) {
            std::throw_with_nested(std::runtime_error("failed to validate 'image_ids'"));
        }

        try {
            auto scale_handle = ghandle.openDataSet("image_scale_factors");
            if (ritsuko::hdf5::exceeds_float_limit(scale_handle, 64)) {
                throw std::runtime_error("expected a datatype for 'image_scale_factors' that fits in a 64-bit float");
            }

            auto scale_space = scale_handle.getSpace(); 
            if (scale_space.getSimpleExtentNdims() != 1) {
                throw std::runtime_error("expected a 1-dimensional dataset");
            }

            hsize_t num_scale;
            scale_space.getSimpleExtentDims(&num_scale);
            if (num_scale != num_images) {
                throw std::runtime_error("expected length to be the same as that of 'image_samples'");
            }

            ritsuko::hdf5::Stream1dNumericDataset<double> scale_stream(
                &scale_handle,
                num_images,
                [&]{
                    ritsuko::hdf5::Stream1dNumericDatasetOptions opt;
                    opt.contiguous_chunk_size = options.hdf5_buffer_size;
                    return opt;
                }()
            );

            iterate_stream<double>(
                scale_stream,
                [&](hsize_t, double sc) {
                    if (!std::isfinite(sc) || sc <= 0) {
                        throw std::runtime_error("entries should be finite and positive");
                    }
                }
            );
        } catch (...) {
            std::throw_with_nested(std::runtime_error("failed to validate 'image_scale_factors'"));
        }

        if (version.ge(1, 3, 0) && !ghandle.exists("image_formats")) { 
            image_formats.resize(num_images, "OTHER");
        } else {
            try {
                auto format_handle = ghandle.openDataSet("image_formats");
                if (!ritsuko::hdf5::is_utf8_string(format_handle)) {
                    throw std::runtime_error("expected a datatype that can be represented by a UTF-8 encoded string");
                }

                auto format_space = format_handle.getSpace();
                if (format_space.getSimpleExtentNdims() != 1) {
                    throw std::runtime_error("expected a 1-dimensional dataset");
                }

                hsize_t num_format;
                format_space.getSimpleExtentDims(&num_format);
                if (num_format != num_images) {
                    throw std::runtime_error("length should be the same as that of 'image_samples'");
                }

                image_formats.reserve(num_images);
                ritsuko::hdf5::Stream1dStringDataset format_stream(
                    &format_handle,
                    num_images, 
                    [&]{
                        ritsuko::hdf5::Stream1dStringDatasetOptions opt;
                        opt.contiguous_chunk_size = options.hdf5_buffer_size;
                        return opt;
                    }()
                );

                iterate_stream<std::string>(
                    format_stream,
                    [&](hsize_t, std::string fmt) -> void {
                        image_formats.push_back(std::move(fmt));
                    }
                );
            } catch (...) {
                std::throw_with_nested(std::runtime_error("failed to validate 'image_formats'"));
            }
        }
    } catch (...) {
        std::throw_with_nested(std::runtime_error("failed to validate 'mapping.h5'"));
    }

    // Now validating the images themselves. 
    const auto num_images = image_formats.size();
    for (I<decltype(num_images)> i = 0; i < num_images; ++i) {
        auto ipath = image_dir / ritsuko::itoa(i);
        const auto& format = image_formats[i];
        try {
            if (format == "PNG") {
                ipath += ".png";
                validate_png_image(ipath);
            } else if (format == "TIFF") {
                ipath += ".tif";
                validate_tiff_image(ipath);
            } else if (format == "OTHER" && version.ge(1, 1, 0)) {
                auto imeta = read_object_metadata(ipath);
                if (!satisfies_interface(imeta.type, "IMAGE", options)) {
                    throw std::runtime_error("object should satisfy the 'IMAGE' interface");
                }
                validate(ipath, imeta, options);
            } else {
                throw std::runtime_error("image format '" + format + "' is not currently supported");
            }
        } catch (...) {
            std::throw_with_nested(std::runtime_error("failed to validate image " + ritsuko::itoa(i)));
        }
    }

    auto num_dir_obj = count_directory_entries(image_dir);
    if (!sanisizer::is_equal(num_dir_obj - 1, num_images)) { // -1 to account for the mapping.h5 file itself.
        throw std::runtime_error("more objects than expected inside the subdirectory");
    }
}
/**
 * @endcond
 */

/**
 * @param path Path to the directory containing the spatial experiment.
 * @param metadata Metadata for the object, typically read from its `OBJECT` file.
 * @param options Validation options.
 */
inline void validate_spatial_experiment(const std::filesystem::path& path, const ObjectMetadata& metadata, const Options& options) {
    validate_single_cell_experiment(path, metadata, options);

    const std::string type_name = "spatial_experiment"; // use a separate variable to avoid dangling reference warnings from GCC.
    ritsuko::Version version;
    try {
        const auto& type_meta = extract_json_object(metadata.other, type_name);
        try {
            const std::string& vstring = extract_json_version_string(type_meta);
            version = ritsuko::parse_version_string(vstring.c_str(), vstring.size(), /* skip_patch = */ true);
            if (version.major != 1) {
                throw std::runtime_error("unsupported version string '" + vstring + "'");
            }
        } catch (...) {
            std::throw_with_nested(std::runtime_error("failed to validate 'version'"));
        }
    } catch (...) {
        std::throw_with_nested(std::runtime_error("failed to validate '" + type_name + " in the object metadata"));
    }

    auto dims = dimensions_of_summarized_experiment(path, metadata, options);

    try {
        validate_spatial_coordinates(path / "coordinates", dims[1], options);
    } catch (...) {
        std::throw_with_nested(std::runtime_error("failed to validate 'coordinates'"));
    }

    try {
        validate_spatial_images(path / "images", dims[1], version, options);
    } catch (...) {
        std::throw_with_nested(std::runtime_error("failed to validate 'images'"));
    }
}

}

#endif
