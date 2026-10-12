#ifndef MOCK_SPATIAL_EXPERIMENT_H
#define MOCK_SPATIAL_EXPERIMENT_H

#include <vector>
#include <string>
#include <numeric>
#include <optional>

#include "H5Cpp.h"
#include "utils.h"
#include "mock_single_cell_experiment.h"

struct SpatialExperimentOptions {
    SpatialExperimentOptions(std::size_t nr, std::size_t nc) : base(nr, nc) {}
    SingleCellExperimentOptions base;
    std::size_t coordinate_dimensions = 2;
    std::size_t num_samples = 1;
    std::size_t num_images_per_sample = 1;
    std::optional<std::string> version;
    bool add_images = true;
};

inline void add_spatial_experiment_metadata(millijson::Base* input, const std::optional<std::string>& version) {
    auto& remap = reinterpret_cast<millijson::Object*>(input)->value();
    auto optr = new millijson::Object({});
    remap["spatial_experiment"] = std::shared_ptr<millijson::Base>(optr);

    std::shared_ptr<millijson::Base> vptr;
    if (version.has_value()) {
        vptr.reset(new millijson::String(*version));
    } else {
        vptr.reset(new millijson::String("1.0"));
    }
    optr->value()["version"] = std::move(vptr);
}

inline const std::vector<unsigned char>& get_png_signature() {
    static std::vector<unsigned char> png_sig{ 137, 80, 78, 71, 13, 10, 26, 10 };
    return png_sig;
}

inline H5::Group mock_spatial_experiment(const std::filesystem::path& dir, const SpatialExperimentOptions& options) {
    mock_single_cell_experiment(dir, options.base);

    auto opath = dir / "OBJECT";
    {
        auto parsed = millijson::parse_file(opath.c_str(), {});
        auto& remap = reinterpret_cast<millijson::Object*>(parsed.get())->value();
        remap["type"] = std::shared_ptr<millijson::Base>(new millijson::String("spatial_experiment"));
        add_spatial_experiment_metadata(parsed.get(), options.version);
        dump_json(parsed.get(), opath);
    }

    {
        std::vector<hsize_t> dims(2);
        dims[0] = options.base.base.base.num_cols;
        dims[1] = options.coordinate_dimensions;
        mock_dense_array(dir / "coordinates", DenseArrayType::NUMBER, dims);
    }

    auto idir = dir / "images";
    std::filesystem::create_directory(idir);
    H5::H5File mhandle(idir / "mapping.h5", H5F_ACC_TRUNC);
    auto ghandle = mhandle.createGroup("spatial_experiment");

    {
        std::vector<std::string> sample_names;
        sample_names.reserve(options.num_samples);
        for (size_t s = 0; s < options.num_samples; ++s) {
            sample_names.push_back("SAMPLE_" + std::to_string(s + 1));
        }
        add_hdf5_string_dataset(ghandle, "sample_names", sample_names);
    }

    {
        std::vector<int> assignments;
        assignments.reserve(options.base.base.base.num_cols);
        for (size_t c = 0; c < options.base.base.base.num_cols; ++c) {
            assignments.push_back(c % options.num_samples);
        }
        add_hdf5_numeric_dataset(ghandle, "column_samples", H5::PredType::NATIVE_UINT16, assignments);
    }

    std::size_t num_images;
    {
        std::vector<int> image_assignments;
        std::vector<std::string> image_ids;
        std::vector<double> image_scales;
        for (size_t i = 0; i < options.num_images_per_sample; ++i) {
            for (size_t s = 0; s < options.num_samples; ++s) {
                image_ids.push_back("IMAGE_" + std::to_string(i + 1));
                image_assignments.push_back(s);
                image_scales.push_back(0.1 * static_cast<double>(s + 1) * static_cast<double>(i + 1));
            }
        }
        add_hdf5_numeric_dataset(ghandle, "image_samples", H5::PredType::NATIVE_UINT8, image_assignments);
        add_hdf5_numeric_dataset(ghandle, "image_scale_factors", H5::PredType::NATIVE_DOUBLE, image_scales);
        add_hdf5_string_dataset(ghandle, "image_ids", image_ids);
        num_images = image_ids.size();
    }

    if (options.add_images) {
        const auto& png_sig = get_png_signature();
        for (std::size_t i = 0; i < num_images; ++i) {
            std::string path = idir / (std::to_string(i) + ".png");
            byteme::RawFileWriter writer(path.c_str(), {});
            writer.write(png_sig.data(), png_sig.size());
        }

        std::vector<std::string> formats(num_images, "PNG");
        add_hdf5_string_dataset(ghandle, "image_formats", formats);
    }

    return ghandle;
}

#endif
