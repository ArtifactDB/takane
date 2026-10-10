#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "mock_ranged_summarized_experiment.h"
#include "utils.h"

#include <string>
#include <vector>
#include <filesystem>
#include <stdexcept>

TEST(RangedSummarizedExperiment, Okay) {
    auto dir = define_test_path("ranged_summarized_experiment");

    {
        mock_ranged_summarized_experiment(dir, RangedSummarizedExperimentOptions(39, 23));
    }
    {
        test_validate(dir);
        EXPECT_EQ(test_height(dir), 39);
        EXPECT_EQ(test_dimensions(dir), (std::vector<std::size_t>{ 39, 23 }));
    }

    // With a GRL.
    {
        RangedSummarizedExperimentOptions opt(55, 14);
        opt.use_grl = true;
        mock_ranged_summarized_experiment(dir, opt);
    }
    {
        test_validate(dir);
        EXPECT_EQ(test_height(dir), 55);
        EXPECT_EQ(test_dimensions(dir), (std::vector<std::size_t>{ 55, 14 }));
    }

    // With nothing.
    {
        mock_ranged_summarized_experiment(dir, RangedSummarizedExperimentOptions(6, 61));
        std::filesystem::remove_all(dir / "row_ranges");
    }
    {
        test_validate(dir);
        EXPECT_EQ(test_height(dir), 6);
        EXPECT_EQ(test_dimensions(dir), (std::vector<std::size_t>{ 6, 61 }));
    }
}

TEST(RangedSummarizedExperiment, BaseError) {
    auto dir = define_test_path("ranged_summarized_experiment");

    // Check that the base SE is actually validated.
    {
        RangedSummarizedExperimentOptions opt(17, 40);
        opt.has_row_data = false;
        mock_ranged_summarized_experiment(dir, opt);
        mock_data_frame(dir / "row_data", 15, {});
    }
    expect_validation_error(dir, "number of rows");
}

TEST(RangedSummarizedExperiment, VersionError) {
    auto dir = define_test_path("ranged_summarized_experiment");

    {
        mock_ranged_summarized_experiment(dir, RangedSummarizedExperimentOptions(12, 40));

        std::string objpath = dir / "OBJECT";
        auto contents = millijson::parse_file(objpath.c_str(), {});
        auto optr = reinterpret_cast<millijson::Object*>(contents.get());
        add_ranged_summarized_experiment_metadata(optr, "2.0");
        dump_json(contents.get(), dir / "OBJECT");
    }
    expect_validation_error(dir, "unsupported version");
}

TEST(RangedSummarizedExperiment, RowRangesError) {
    auto dir = define_test_path("ranged_summarized_experiment");

    {
        mock_ranged_summarized_experiment(dir, RangedSummarizedExperimentOptions(16, 41));
        std::filesystem::remove_all(dir / "row_ranges");
        mock_data_frame(dir / "row_ranges", 16, {});
    }
    expect_validation_error(dir, "'genomic_ranges', 'genomic_ranges_list'");

    {
        mock_ranged_summarized_experiment(dir, RangedSummarizedExperimentOptions(16, 41));
        std::filesystem::remove_all(dir / "row_ranges");
        mock_genomic_ranges(dir / "row_ranges", 17, 4);
    }
    expect_validation_error(dir, "number of rows");
}
