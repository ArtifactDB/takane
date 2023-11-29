#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "takane/takane.hpp"

#include "ranged_summarized_experiment.h"
#include "utils.h"

#include <string>
#include <vector>
#include <filesystem>
#include <stdexcept>

struct RangedSummarizedExperimentTest : public ::testing::Test {
    RangedSummarizedExperimentTest() {
        dir = "TEST_ranged_summarized_experiment";
        name = "ranged_summarized_experiment";
    }

    std::filesystem::path dir;
    std::string name;

    void expect_error(const std::string& msg) {
        EXPECT_ANY_THROW({
            try {
                takane::validate(dir);
            } catch (std::exception& e) {
                EXPECT_THAT(e.what(), ::testing::HasSubstr(msg));
                throw;
            }
        });
    }
};

TEST_F(RangedSummarizedExperimentTest, BaseChecks) {
    {
        initialize_directory(dir, name);
        std::ofstream handle(dir / "summarized_experiment.json");
        handle << "[]";
    }
    expect_error("top-level object");

    // With a GRL:
    ranged_summarized_experiment::mock(dir, ranged_summarized_experiment::Options(20, 15, true));
    takane::validate(dir); 
    EXPECT_EQ(takane::height(dir), 20);

    // With a GRanges:
    ranged_summarized_experiment::mock(dir, ranged_summarized_experiment::Options(30, 9, false));
    takane::validate(dir); 
    std::vector<size_t> expected_dim{ 30, 9 };
    EXPECT_EQ(takane::dimensions(dir), expected_dim);
}

TEST_F(RangedSummarizedExperimentTest, RowRanges) {
    ranged_summarized_experiment::mock(dir, ranged_summarized_experiment::Options(10, 15, true));

    {
        data_frame::mock(dir / "row_ranges", 10, {});
    }
    expect_error("must be a 'genomic_ranges' or 'genomic_ranges_list'");

    {
        genomic_ranges::mock(dir / "row_ranges", 20, 10);
    }
    expect_error("length equal to the number of rows");

    // Absence of row ranges is allowed, in which case we are assumed
    // to have non-informative ranges for the RangedSummarizedExperiment.
    {
        std::filesystem::remove_all(dir / "row_ranges");
    }
    takane::validate(dir); 
}
