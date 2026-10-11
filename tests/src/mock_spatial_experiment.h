#ifndef MOCK_SPATIAL_EXPERIMENT_H
#define MOCK_SPATIAL_EXPERIMENT_H

#include <vector>
#include <string>
#include <numeric>

#include "H5Cpp.h"
#include "utils.h"
#include "mock_single_cell_experiment.h"

struct SpatialExperimentOptions : public SingleCellExperimentOptions {
    SpatialExperimentOptions(std::size_t nr, std::size_t nc) : SingleCellExperimentOptions(nr, nc) {}
    std::size_t num_samples = 1;
    std::size_t num_images_per_sample = 1;
    bool allow_other = true;
    bool specify_format = true;
    std::string version;
};

inline void add_spatial_experiment_metadata(millijson::Base* input, const std::string& version) {
    auto& remap = reinterpret_cast<millijson::Object*>(input)->value();
    auto optr = new millijson::Object({});
    remap["spatial_experiment"] = std::shared_ptr<millijson::Base>(optr);
    optr->value()["version"] = std::shared_ptr<millijson::Base>(new millijson::String(version));
}

inline void mock_spatial_experiment(const std::filesystem::path& dir, const SpatialExperimentOptions& options) {
    mock_single_cell_experiment(dir, options);

    auto version = options.version;
    if (options.allow_other) {
        version = "1.1";
    }
    if (!options.specify_format) {
        version = "1.3";
    }

    auto opath = dir / "OBJECT";
    {
        auto parsed = millijson::parse_file(opath.c_str(), {});
        auto& remap = reinterpret_cast<millijson::Object*>(parsed.get())->value();
        remap["type"] = std::shared_ptr<millijson::Base>(new millijson::String("spatial_experiment"));
        add_spatial_experiment_metadata(parsed.get(), version);
        dump_json(parsed.get(), opath);
    }

    {
        std::vector<hsize_t> dims(2);
        dims[0] = options.num_cols;
        dims[1] = 2;
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
        assignments.reserve(options.num_cols);
        for (size_t c = 0; c < options.num_cols; ++c) {
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
                image_scales.push_back(0.1);
            }
        }
        add_hdf5_numeric_dataset(ghandle, "image_samples", H5::PredType::NATIVE_UINT8, image_assignments);
        add_hdf5_numeric_dataset(ghandle, "image_scale_factors", H5::PredType::NATIVE_DOUBLE, image_scales);
        add_hdf5_string_dataset(ghandle, "image_ids", image_ids);
        num_images = image_ids.size();
    }

    std::vector<std::vector<unsigned char> > sim_signatures{
        std::vector<unsigned char>{ 137, 80, 78, 71, 13, 10, 26, 10 }, // png
        std::vector<unsigned char>{ 77, 77, 0, 42 }, // tif (1)
        std::vector<unsigned char>{ 73, 73, 42, 0 } // tif (2)
    };
    std::vector<std::string> sim_formats { "PNG", "TIFF", "TIFF" };
    std::vector<std::string> sim_extensions { ".png", ".tif", ".tif" };

    if (options.specify_format) {
        std::vector<std::string> formats;
        formats.reserve(num_images);
        for (std::size_t i = 0; i < num_images; ++i) {
            auto path = idir / std::to_string(i);

            if (options.allow_other && i % 4) {
                std::filesystem::create_directory(path);
                quick_text_write(
                    path / "OBJECT",
                    "{ \"type\": \"image_file\", \"image_file\": { \"version\": \"1.0\", \"format\": \"" + sim_formats[i % 3] + "\" } }"
                );
                path /= "file" + sim_extensions[i % 3];
                formats.push_back("OTHER");
            } else {
                path += sim_extensions[i % 3]; 
                formats.push_back(sim_formats[i % 3]);
            }

            byteme::RawFileWriter writer(path.c_str(), {});
            const auto& cur_sig = sim_signatures[i % 3];
            writer.write(cur_sig.data(), cur_sig.size());
        }

        add_hdf5_string_dataset(ghandle, "image_formats", formats);
    } else {
        for (std::size_t i = 0; i < num_images; ++i) {
            auto path = idir / std::to_string(i);
            std::filesystem::create_directory(path);
            quick_text_write(
                path / "OBJECT",
                "{ \"type\": \"image_file\", \"image_file\": { \"version\": \"1.0\", \"format\": \"" + sim_formats[i % 3] + "\" } }"
            );
            path /= "file" + sim_extensions[i % 3];
            byteme::RawFileWriter writer(path.c_str(), {});
            const auto& cur_sig = sim_signatures[i % 3];
            writer.write(cur_sig.data(), cur_sig.size());
        }
    }
}

#endif
