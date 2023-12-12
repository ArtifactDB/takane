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

struct Options {
    Options(size_t nr, size_t nc, size_t na = 1) : num_rows(nr), num_cols(nc), num_assays(na) {}
    size_t num_rows;
    size_t num_cols;
    size_t num_assays;
    bool has_row_data = false;
    bool has_column_data = false;
    bool has_other_data = false;
};

inline void mutate_object_file(const std::filesystem::path& dir, const std::string& type, const std::string extras) {
    std::string str;
    {
        std::ifstream input(dir / "OBJECT");
        std::stringstream stream;
        stream << input.rdbuf(); 
        str = stream.str();
    }

    std::stringstream stream2(dir / "OBJECT");
    auto i = str.find(",");
    auto j = str.rfind("}");
    stream2 << "{ \"type\": \"" + type + "\", " + str.substr(i + 1, j) + ", \"" + type + "\": " + extras + "}";
}

inline void dump_object_file(const std::filesystem::path& dir, size_t num_rows, size_t num_cols) {
    std::ofstream handle(dir / "OBJECT");
    handle << "{ \"type\": \"summarized_experiment\", \"summarized_experiment\": ";
    handle << "{ \"version\": \"1.0\", \"dimensions\": [ " << num_rows << ", " << num_cols << "] }";
    handle << "}";
}

inline void mock(const std::filesystem::path& dir, const Options& options) {
    initialize_directory(dir);
    dump_object_file(dir, options.num_rows, options.num_cols);

    {
        auto adir = dir / "assays";
        std::filesystem::create_directory(adir);

        std::ofstream handle(adir / "names.json");
        handle << "[";
        for (size_t a = 0; a < options.num_assays; ++a) {
            if (a != 0) {
                handle << ", ";
            }
            auto aname = std::to_string(a);
            handle << "\"assay-" << aname << "\"";
            dense_array::mock(adir / aname, dense_array::Type::INTEGER, { static_cast<hsize_t>(options.num_rows), static_cast<hsize_t>(options.num_cols) });
        }
        handle << "]";
    }

    if (options.has_row_data) {
        data_frame::mock(dir / "row_data", options.num_rows, {});
    }

    if (options.has_column_data) {
        data_frame::mock(dir / "column_data", options.num_cols, {});
    }

    if (options.has_other_data) {
        simple_list::mock(dir / "other_data");
    }
}

}

#endif
