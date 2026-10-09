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
        mock_ranged_summarized_experiment(dir, RangedSummarizedExperimentOptions(39, 23, false));
    }
    {
        test_validate(dir);
        EXPECT_EQ(test_height(dir), 99);
        EXPECT_EQ(test_dimensions(dir), (std::vector<std::size_t>{ 39, 23 }));
    }

    // With a GRL.
    {
        mock_ranged_summarized_experiment(dir, RangedSummarizedExperimentOptions(55, 14, true));
    }
    {
        test_validate(dir);
        EXPECT_EQ(test_height(dir), 55);
        EXPECT_EQ(test_dimensions(dir), (std::vector<std::size_t>{ 55, 14 }));
    }

    // With nothing.
    {
        mock_ranged_summarized_experiment(dir, RangedSummarizedExperimentOptions(6, 61, false));
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
        RangedSummarizedExperimentOptions opt(17, 40, false);
        opt.has_row_data = false;
        mock_ranged_summarized_experiment(dir, opt);
        mock_data_frame(dir / "row_data", 15, {});
    }
    expect_validation_error(dir, "number of rows");
}

TEST(RangedSummarizedExperiment, VersionError) {
    auto dir = define_test_path("ranged_summarized_experiment");

    {
        mock_ranged_summarized_experiment(dir, RangedSummarizedExperimentOptions(12, 40, false));

        auto optr = new millijson::Object({});
        std::shared_ptr<millijson::Base> contents(optr);
        optr->value()["type"] = std::shared_ptr<millijson::Base>(new millijson::String("ranged_summarized_experiment"));
        add_summarized_experiment_metadata(contents.get(), "1.0", 12, 40);
        add_ranged_summarized_experiment_metadata(contents.get(), "2.0");
        dump_json(contents.get(), dir / "OBJECT");
    }
    expect_validation_error(dir, "unsupported version");
}

TEST(RangedSummarizedExperiment, RowRangesError) {
    auto dir = define_test_path("ranged_summarized_experiment");

    {
        mock_ranged_summarized_experiment(dir, RangedSummarizedExperimentOptions(16, 41, false));
        std::filesystem::remove_all(dir / "row_ranges");
        mock_data_frame(dir / "row_ranges", 16, {});
    }
    expect_validation_error(dir, "'genomic_ranges', 'genomic_ranges_list'");

    {
        mock_ranged_summarized_experiment(dir, RangedSummarizedExperimentOptions(16, 41, false));
        std::filesystem::remove_all(dir / "row_ranges");
        mock_genomic_ranges(dir / "row_ranges", 17, 4);
    }
    expect_validation_error(dir, "number of rows");
}
