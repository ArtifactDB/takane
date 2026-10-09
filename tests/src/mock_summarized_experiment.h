#ifndef MOCK_SUMMARIZED_EXPERIMENT_H
#define MOCK_SUMMARIZED_EXPERIMENT_H

#include <vector>
#include <string>
#include <numeric>
#include <cstddef>

#include "H5Cpp.h"
#include "utils.h"
#include "mock_data_frame.h"
#include "mock_simple_list.h"
#include "mock_dense_array.h"

struct SummarizedExperimentOptions {
    SummarizedExperimentOptions(std::size_t nr, std::size_t nc, std::size_t na = 1) : num_rows(nr), num_cols(nc), num_assays(na) {}
    std::size_t num_rows;
    std::size_t num_cols;
    std::size_t num_assays;
    bool has_row_data = false;
    bool has_column_data = false;
    bool has_other_data = false;
};

inline void add_summarized_experiment_metadata(millijson::Base* input, const std::string& version, size_t num_rows, size_t num_cols) {
    auto& remap = reinterpret_cast<millijson::Object*>(input)->value();
    auto optr = new millijson::Object({});
    remap["summarized_experiment"] = std::shared_ptr<millijson::Base>(optr);
    optr->value()["version"] = std::shared_ptr<millijson::Base>(new millijson::String(version));

    auto aptr = new millijson::Array({});
    optr->value()["dimensions"] = std::shared_ptr<millijson::Base>(aptr);
    aptr->value().emplace_back(new millijson::Number(num_rows));
    aptr->value().emplace_back(new millijson::Number(num_cols));
}

inline void mock_summarized_experiment(const std::filesystem::path& dir, const SummarizedExperimentOptions& options) {
    initialize_directory(dir);

    auto optr = new millijson::Object({});
    std::shared_ptr<millijson::Base> contents(optr);
    optr->value()["type"] = std::shared_ptr<millijson::Base>(new millijson::String("summarized_experiment"));
    add_summarized_experiment_metadata(contents.get(), "1.0", options.num_rows, options.num_cols);
    dump_json(contents.get(), dir / "OBJECT");

    {
        auto adir = dir / "assays";
        std::filesystem::create_directory(adir);
        std::ofstream handle(adir / "names.json");
        handle << "[";

        for (std::size_t a = 0; a < options.num_assays; ++a) {
            if (a != 0) {
                handle << ", ";
            }
            auto aname = std::to_string(a);
            handle << "\"assay-" << aname << "\"";
            mock_dense_array(
                adir / aname,
                DenseArrayType::INTEGER,
                { static_cast<hsize_t>(options.num_rows), static_cast<hsize_t>(options.num_cols) }
            );
        }
        handle << "]";
    }

    if (options.has_row_data) {
        mock_data_frame(dir / "row_data", options.num_rows, {});
    }

    if (options.has_column_data) {
        mock_data_frame(dir / "column_data", options.num_cols, {});
    }

    if (options.has_other_data) {
        mock_simple_list(dir / "other_data");
    }
}

#endif
