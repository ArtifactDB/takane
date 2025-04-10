#ifndef SPATIAL_EXPERIMENT_H
#define SPATIAL_EXPERIMENT_H

#include <vector>
#include <string>
#include <numeric>

#include "H5Cpp.h"
#include "utils.h"
#include "single_cell_experiment.h"

namespace spatial_experiment {

struct Options : public ::single_cell_experiment::Options {
    Options(size_t nr, size_t nc, size_t ns = 1, size_t ni = 1) : ::single_cell_experiment::Options(nr, nc), num_samples(ns), num_images_per_sample(ni) {}
    size_t num_samples;
    size_t num_images_per_sample;
};

inline void add_object_metadata(millijson::Base* input, const std::string& version) {
    auto& remap = reinterpret_cast<millijson::Object*>(input)->values;
    auto optr = new millijson::Object;
    remap["spatial_experiment"] = std::shared_ptr<millijson::Base>(optr);
    optr->add("version", std::shared_ptr<millijson::Base>(new millijson::String(version)));
}

inline void mock(const std::filesystem::path& dir, const Options& options) {
    ::single_cell_experiment::mock(dir, options);

    auto opath = dir / "OBJECT";
    {
        auto parsed = millijson::parse_file(opath.c_str());
        auto& remap = reinterpret_cast<millijson::Object*>(parsed.get())->values;
        remap["type"] = std::shared_ptr<millijson::Base>(new millijson::String("spatial_experiment"));
        add_object_metadata(parsed.get(), "1.0");
        json_utils::dump(parsed.get(), opath);
    }

    {
        std::vector<hsize_t> dims(2);
        dims[0] = options.num_cols;
        dims[1] = 2;
        dense_array::mock(dir / "coordinates", dense_array::Type::NUMBER, dims);
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
        hdf5_utils::spawn_string_data(ghandle, "sample_names", 10, sample_names);
    }

    {
        std::vector<int> assignments;
        assignments.reserve(options.num_cols);
        for (size_t c = 0; c < options.num_cols; ++c) {
            assignments.push_back(c % options.num_samples);
        }
        hdf5_utils::spawn_numeric_data(ghandle, "column_samples", H5::PredType::NATIVE_UINT16, assignments);
    }

    size_t num_images;
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
        hdf5_utils::spawn_numeric_data(ghandle, "image_samples", H5::PredType::NATIVE_UINT8, image_assignments);
        hdf5_utils::spawn_numeric_data(ghandle, "image_scale_factors", H5::PredType::NATIVE_DOUBLE, image_scales);
        hdf5_utils::spawn_string_data(ghandle, "image_ids", H5T_VARIABLE, image_ids);
        num_images = image_ids.size();
    }

    {
        std::vector<std::string> formats;
        formats.reserve(num_images);
        for (size_t i = 0; i < num_images; ++i) {
            std::vector<unsigned char> magical;
            auto path = idir / std::to_string(i);

            switch (i%3) {
                case 0:
                    formats.push_back("PNG");
                    magical = std::vector<unsigned char>{ 137, 80, 78, 71, 13, 10, 26, 10 };
                    path += ".png";
                    break;
                case 1:
                    formats.push_back("TIFF");
                    magical = std::vector<unsigned char>{ 77, 77, 0, 42 }; 
                    path += ".tif";
                    break;
                case 2:
                    formats.push_back("TIFF");
                    magical = std::vector<unsigned char>{ 73, 73, 42, 0 };
                    path += ".tif";
                    break;
            };

            magical.push_back(0); // just adding something past the signature.
            byteme::RawFileWriter writer(path.c_str(), {});
            writer.write(magical.data(), magical.size());
        }

        hdf5_utils::spawn_string_data(ghandle, "image_formats", 5, formats);
    }
}

}

#endif
