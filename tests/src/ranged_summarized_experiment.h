#ifndef RANGED_SUMMARIZED_EXPERIMENT_H
#define RANGED_SUMMARIZED_EXPERIMENT_H

#include <vector>
#include <string>
#include <numeric>

#include "H5Cpp.h"
#include "utils.h"
#include "summarized_experiment.h"
#include "genomic_ranges.h"

namespace ranged_summarized_experiment {

struct Options : public ::summarized_experiment::Options {
    Options(size_t nr, size_t nc, bool grl = false) : ::summarized_experiment::Options(nr, nc), use_grl(grl) {}
    bool use_grl = false;
};

inline void add_object_metadata(millijson::Base* input, const std::string& version) {
    auto& remap = reinterpret_cast<millijson::Object*>(input)->value();
    auto optr = new millijson::Object({});
    remap["ranged_summarized_experiment"] = std::shared_ptr<millijson::Base>(optr);
    optr->value()["version"] = std::shared_ptr<millijson::Base>(new millijson::String(version));
}

inline void mock(const std::filesystem::path& dir, const Options& options) {
    ::summarized_experiment::mock(dir, options);

    auto opath = dir / "OBJECT";
    {
        auto parsed = millijson::parse_file(opath.c_str(), {});
        auto& remap = reinterpret_cast<millijson::Object*>(parsed.get())->value();
        remap["type"] = std::shared_ptr<millijson::Base>(new millijson::String("ranged_summarized_experiment"));
        add_object_metadata(parsed.get(), "1.0");
        json_utils::dump(parsed.get(), opath);
    }

    auto rdir = dir / "row_ranges";
    if (options.use_grl) {
        initialize_directory_simple(rdir, "genomic_ranges_list", "1.0");
        H5::H5File handle(rdir / "partitions.h5", H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup("genomic_ranges_list");
        hdf5_utils::attach_attribute(ghandle, "version", "1.0");

        std::vector<int> lengths(options.num_rows, 2);
        hdf5_utils::spawn_numeric_data<int>(ghandle, "lengths", H5::PredType::NATIVE_UINT32, lengths);
        genomic_ranges::mock(rdir / "concatenated", 2 * options.num_rows, 10);
    } else {
        ::genomic_ranges::mock(dir / "row_ranges", options.num_rows, 10);
    }
}

}

#endif
