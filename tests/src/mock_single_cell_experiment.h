#ifndef MOCK_SINGLE_CELL_EXPERIMENT_H
#define MOCK_SINGLE_CELL_EXPERIMENT_H

#include <vector>
#include <string>
#include <numeric>
#include <cstddef>

#include "H5Cpp.h"

#include "utils.h"
#include "mock_ranged_summarized_experiment.h"
#include "mock_genomic_ranges.h"

struct SingleCellExperimentOptions : public RangedSummarizedExperimentOptions {
    SingleCellExperimentOptions(std::size_t nr, std::size_t nc) : RangedSummarizedExperimentOptions(nr, nc) {}
    std::size_t num_reduced_dims = 1;
    std::size_t num_alt_exps = 1;
    std::string main_exp_name;
};

inline void add_single_cell_experiment_metadata(millijson::Base* input, const std::string& version, const std::string& main_exp_name) {
    auto& remap = reinterpret_cast<millijson::Object*>(input)->value();
    auto optr = new millijson::Object({});
    remap["single_cell_experiment"] = std::shared_ptr<millijson::Base>(optr);
    optr->value()["version"] = std::shared_ptr<millijson::Base>(new millijson::String(version));
    if (!main_exp_name.empty()) {
        optr->value()["main_experiment_name"] = std::shared_ptr<millijson::Base>(new millijson::String(main_exp_name));
    }
}

inline void mock_single_cell_experiment(const std::filesystem::path& dir, const SingleCellExperimentOptions& options) {
    mock_ranged_summarized_experiment(dir, options);

    auto opath = dir / "OBJECT";
    {
        auto parsed = millijson::parse_file(opath.c_str(), {});
        auto& remap = reinterpret_cast<millijson::Object*>(parsed.get())->value();
        remap["type"] = std::shared_ptr<millijson::Base>(new millijson::String("single_cell_experiment"));
        add_single_cell_experiment_metadata(parsed.get(), "1.0", options.main_exp_name);
        dump_json(parsed.get(), opath);
    }

    {
        auto rddir = dir / "reduced_dimensions";
        std::filesystem::create_directory(rddir);

        std::ofstream handle(rddir / "names.json");
        handle << "[";
        for (std::size_t rd = 0; rd < options.num_reduced_dims; ++rd) {
            if (rd != 0) {
                handle << ", ";
            }
            auto rdname = std::to_string(rd);
            handle << "\"reddim-" << rdname << "\"";
            mock_dense_array(rddir / rdname, DenseArrayType::NUMBER, { static_cast<hsize_t>(options.num_cols), static_cast<hsize_t>(2) });
        }
        handle << "]";
    }

    {
        auto aedir = dir / "alternative_experiments";
        std::filesystem::create_directory(aedir);

        std::ofstream handle(aedir / "names.json");
        handle << "[";
        for (std::size_t ae = 0; ae < options.num_alt_exps; ++ae) {
            if (ae != 0) {
                handle << ", ";
            }
            auto aename = std::to_string(ae);
            handle << "\"altexps-" << aename << "\"";
            mock_summarized_experiment(aedir / aename, SummarizedExperimentOptions((ae + 1) * 10, options.num_cols));
        }
        handle << "]";
    }
}

#endif
