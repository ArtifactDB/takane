#ifndef MULTI_SAMPLE_DATASET_H 
#define MULTI_SAMPLE_DATASET_H

#include <vector>
#include <string>
#include <numeric>

#include "H5Cpp.h"
#include "utils.h"
#include "summarized_experiment.h"

namespace multi_sample_dataset {

struct Options {
    Options(size_t ns) : num_samples(ns) {}
    size_t num_samples;
    std::vector<summarized_experiment::Options> experiments;
};

inline void mock(const std::filesystem::path& dir, const Options& options) {
    initialize_directory_simple(dir, "multi_sample_dataset", "1.0");

    // Setting up an experiment.
    if (options.experiments.size()) {
        auto edir = dir / "experiments";
        std::filesystem::create_directory(edir);
        std::ofstream handle(edir / "names.json");
        handle << "[";

        for (size_t e = 0, num_exp = options.experiments.size(); e < num_exp; ++e) {
            auto ename = std::to_string(e);
            summarized_experiment::mock(edir / ename, options.experiments[e]);
            if (e != 0) {
                handle << ", ";
            }
            handle << "\"experiment-" << ename << "\"";
        }
        handle << "]";
    }

    // Setting up the sample data.
    data_frame::mock(dir / "sample_data", options.num_samples, {});

    // Setting up the sample mapping.
    {
        H5::H5File handle(dir / "sample_map.h5", H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup("multi_sample_dataset");

        for (size_t e = 0, num_exp = options.experiments.size(); e < num_exp; ++e) {
            const auto& exp_options = options.experiments[e];

            std::vector<int> dump(exp_options.num_cols);
            for (size_t s = 0; s < exp_options.num_cols; ++s) {
                dump.push_back(s % options.num_samples);
            }

            auto dhandle = hdf5_utils::spawn_data(ghandle, std::to_string(e), exp_options.num_cols, H5::PredType::NATIVE_UINT16);
            dhandle.write(dump.data(), H5::PredType::NATIVE_INT);
        }
    }
}

}

#endif
