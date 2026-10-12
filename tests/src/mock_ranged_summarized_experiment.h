#ifndef RANGED_SUMMARIZED_EXPERIMENT_H
#define RANGED_SUMMARIZED_EXPERIMENT_H

#include <vector>
#include <string>
#include <numeric>
#include <optional>

#include "utils.h"
#include "mock_summarized_experiment.h"
#include "mock_genomic_ranges.h"

enum class RangedSummarizedExperimentRowRangesType { NONE, GRANGES, GRANGESLIST };

struct RangedSummarizedExperimentOptions {
    RangedSummarizedExperimentOptions(std::size_t nr, std::size_t nc) : base(nr, nc) {}
    SummarizedExperimentOptions base;
    RangedSummarizedExperimentRowRangesType row_ranges = RangedSummarizedExperimentRowRangesType::GRANGES;
    std::optional<std::string> version;
};

inline void add_ranged_summarized_experiment_metadata(millijson::Base* input, const std::optional<std::string>& version) {
    auto& remap = reinterpret_cast<millijson::Object*>(input)->value();
    auto optr = new millijson::Object({});
    remap["ranged_summarized_experiment"] = std::shared_ptr<millijson::Base>(optr);

    std::shared_ptr<millijson::Base> vptr;
    if (version.has_value()) {
        vptr.reset(new millijson::String(*version));
    } else {
        vptr.reset(new millijson::String("1.0"));
    }
    optr->value()["version"] = std::move(vptr);
}

inline void mock_ranged_summarized_experiment(const std::filesystem::path& dir, const RangedSummarizedExperimentOptions& options) {
    mock_summarized_experiment(dir, options.base);

    auto opath = dir / "OBJECT";
    {
        auto parsed = millijson::parse_file(opath.c_str(), {});
        auto& remap = reinterpret_cast<millijson::Object*>(parsed.get())->value();
        remap["type"] = std::shared_ptr<millijson::Base>(new millijson::String("ranged_summarized_experiment"));
        add_ranged_summarized_experiment_metadata(parsed.get(), options.version);
        dump_json(parsed.get(), opath);
    }

    auto rdir = dir / "row_ranges";
    if (options.row_ranges == RangedSummarizedExperimentRowRangesType::GRANGESLIST) {
        initialize_directory_simple(rdir, "genomic_ranges_list", "1.0");
        H5::H5File handle(rdir / "partitions.h5", H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup("genomic_ranges_list");
        add_hdf5_string_attribute(ghandle, "version", "1.0");
        std::vector<int> lengths(options.base.num_rows, 2);
        add_hdf5_numeric_dataset(ghandle, "lengths", H5::PredType::NATIVE_UINT32, lengths);
        mock_genomic_ranges(rdir / "concatenated", 2 * options.base.num_rows, 10);
    } else if (options.row_ranges == RangedSummarizedExperimentRowRangesType::GRANGES) {
        mock_genomic_ranges(dir / "row_ranges", options.base.num_rows, 10);
    }
}

#endif
