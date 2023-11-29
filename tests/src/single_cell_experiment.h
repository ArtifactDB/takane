#ifndef SINGLE_CELL_EXPERIMENT_H
#define SINGLE_CELL_EXPERIMENT_H

#include <vector>
#include <string>
#include <numeric>

#include "H5Cpp.h"
#include "utils.h"
#include "ranged_summarized_experiment.h"
#include "genomic_ranges.h"

namespace single_cell_experiment {

struct Options : public ::ranged_summarized_experiment::Options {
    Options(size_t nr, size_t nc, size_t nrd = 1, size_t nae = 1) : ::ranged_summarized_experiment::Options(nr, nc), num_reduced_dims(nrd), num_alt_exps(nae) {}
    size_t num_reduced_dims;
    size_t num_alt_exps;
};

void mock(const std::filesystem::path& dir, const Options& options) {
    ::ranged_summarized_experiment::mock(dir, options);

    {
        std::ofstream ohandle(dir / "OBJECT");
        ohandle << "single_cell_experiment";
    }

    {
        std::ofstream handle(dir / "single_cell_experiment.json");
        handle << "{ \"version\": \"1.0\" }";
    }

    {
        auto rddir = dir / "reduced_dimensions";
        std::filesystem::create_directory(rddir);

        std::ofstream handle(rddir / "names.json");
        handle << "[";
        for (size_t rd = 0; rd < options.num_reduced_dims; ++rd) {
            if (rd != 0) {
                handle << ", ";
            }
            auto rdname = std::to_string(rd);
            handle << "\"reddim-" << rdname << "\"";
            dense_array::mock(rddir / rdname, dense_array::Type::NUMBER, { static_cast<hsize_t>(options.num_cols), static_cast<hsize_t>(2) });
        }
        handle << "]";
    }

    {
        auto aedir = dir / "alternative_experiments";
        std::filesystem::create_directory(aedir);

        std::ofstream handle(aedir / "names.json");
        handle << "[";
        for (size_t ae = 0; ae < options.num_alt_exps; ++ae) {
            if (ae != 0) {
                handle << ", ";
            }
            auto aename = std::to_string(ae);
            handle << "\"altexps-" << aename << "\"";
            summarized_experiment::mock(aedir / aename, ::summarized_experiment::Options((ae + 1) * 10, options.num_cols));
        }
        handle << "]";
    }
}

}

#endif
