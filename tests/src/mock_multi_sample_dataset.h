#ifndef MOCK_MULTI_SAMPLE_DATASET_H 
#define MOCK_MULTI_SAMPLE_DATASET_H

#include <vector>
#include <string>
#include <numeric>

#include "H5Cpp.h"
#include "utils.h"
#include "mock_summarized_experiment.h"

struct MultiSampleDatasetOptions {
    MultiSampleDatasetOptions(std::size_t num_samples_arg, std::vector<SummarizedExperimentOptions> experiments_arg) :
        num_samples(num_samples_arg),
        experiments(std::move(experiments_arg)),
        version("1.0")
    {}
public:
    std::size_t num_samples;
    std::vector<SummarizedExperimentOptions> experiments;
    std::string version;
    bool has_sample_data = true;
};

inline H5::Group mock_multi_sample_dataset(const std::filesystem::path& dir, const MultiSampleDatasetOptions& options) {
    initialize_directory_simple(dir, "multi_sample_dataset", options.version);

    // Setting up an experiment.
    const std::size_t num_exp = options.experiments.size();
    {
        auto edir = dir / "experiments";
        std::filesystem::create_directory(edir);
        std::ofstream handle(edir / "names.json");
        handle << "[";

        for (std::size_t e = 0; e < num_exp; ++e) {
            auto ename = std::to_string(e);
            mock_summarized_experiment(edir / ename, options.experiments[e]);
            if (e != 0) {
                handle << ", ";
            }
            handle << "\"experiment-" << ename << "\"";
        }
        handle << "]";

        // Setting up the sample data.
        if (options.has_sample_data) {
            mock_data_frame(dir / "sample_data", DataFrameOptions(options.num_samples, {}));
        }
    }

    // Setting up the sample mapping.
    H5::H5File handle(dir / "sample_map.h5", H5F_ACC_TRUNC);
    auto ghandle = handle.createGroup("multi_sample_dataset");
    for (std::size_t e = 0; e < num_exp; ++e) {
        const auto& exp_options = options.experiments[e];
        std::vector<int> dump(exp_options.num_cols);
        for (size_t s = 0; s < exp_options.num_cols; ++s) {
            dump[s] = s % options.num_samples;
        }
        add_hdf5_numeric_dataset(ghandle, std::to_string(e), H5::PredType::NATIVE_UINT16, dump);
    }

    return ghandle;
}

#endif
