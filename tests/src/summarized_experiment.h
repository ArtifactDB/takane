#ifndef SUMMARIZED_EXPERIMENT_H
#define SUMMARIZED_EXPERIMENT_H

#include <vector>
#include <string>
#include <numeric>

#include "H5Cpp.h"
#include "utils.h"
#include "data_frame.h"
#include "simple_list.h"
#include "dense_array.h"

namespace summarized_experiment {

inline void mock(const std::filesystem::path& dir, size_t num_rows, size_t num_cols, size_t num_assays, bool has_row_data = false, bool has_column_data = false, bool has_other_data = false) {
    initialize_directory(dir, "summarized_experiment");

    {
        std::ofstream handle(dir / "summarized_experiment.json");
        handle << "{ \"version\": \"1.0\", \"dimensions\": [ " << num_rows << ", " << num_cols << "] }";
    }

    {
        auto adir = dir / "assays";
        std::filesystem::create_directory(adir);

        std::ofstream handle(adir / "names.json");
        handle << "[";
        for (size_t a = 0; a < num_assays; ++a) {
            if (a != 0) {
                handle << ", ";
            }
            auto aname = std::to_string(a);
            handle << "\"assay-" << aname << "\"";
            dense_array::mock(adir / aname, dense_array::Type::INTEGER, { static_cast<hsize_t>(num_rows), static_cast<hsize_t>(num_cols) });
        }
        handle << "]";
    }

    if (has_row_data) {
        data_frame::mock(dir / "row_data", num_rows, {});
    }

    if (has_column_data) {
        data_frame::mock(dir / "column_data", num_cols, {});
    }

    if (has_other_data) {
        simple_list::mock(dir / "other_data");
    }
}

}

#endif
