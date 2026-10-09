#ifndef RANGED_SUMMARIZED_EXPERIMENT_H
#define RANGED_SUMMARIZED_EXPERIMENT_H

#include <vector>
#include <string>
#include <numeric>

#include "utils.h"
#include "mock_summarized_experiment.h"
#include "mock_genomic_ranges.h"

struct RangedSummarizedExperimentOptions : public SummarizedExperimentOptions {
    RangedSummarizedExperimentOptions(size_t nr, size_t nc, bool use_grl) : 
        SummarizedExperimentOptions(nr, nc),
        use_grl(use_grl)
    {}
    bool use_grl = false;
};

inline void add_ranged_summarized_experiment_metadata(millijson::Base* input, const std::string& version) {
    auto& remap = reinterpret_cast<millijson::Object*>(input)->value();
    auto optr = new millijson::Object({});
    remap["ranged_summarized_experiment"] = std::shared_ptr<millijson::Base>(optr);
    optr->value()["version"] = std::shared_ptr<millijson::Base>(new millijson::String(version));
}

inline void mock_ranged_summarized_experiment(const std::filesystem::path& dir, const RangedSummarizedExperimentOptions& options) {
    mock_summarized_experiment(dir, options);

    auto opath = dir / "OBJECT";
    {
        auto parsed = millijson::parse_file(opath.c_str(), {});
        auto& remap = reinterpret_cast<millijson::Object*>(parsed.get())->value();
        remap["type"] = std::shared_ptr<millijson::Base>(new millijson::String("ranged_summarized_experiment"));
        add_ranged_summarized_experiment_metadata(parsed.get(), "1.0");
        dump_json(parsed.get(), opath);
    }

    auto rdir = dir / "row_ranges";
    if (options.use_grl) {
        initialize_directory_simple(rdir, "genomic_ranges_list", "1.0");
        H5::H5File handle(rdir / "partitions.h5", H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup("genomic_ranges_list");
        add_hdf5_string_attribute(ghandle, "version", "1.0");
        std::vector<int> lengths(options.num_rows, 2);
        add_hdf5_numeric_dataset(ghandle, "lengths", H5::PredType::NATIVE_UINT32, lengths);
        mock_genomic_ranges(rdir / "concatenated", 2 * options.num_rows, 10);
    } else {
        mock_genomic_ranges(dir / "row_ranges", options.num_rows, 10);
    }
}

#endif
