#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "takane/utils_summarized_experiment.hpp"

#include "summarized_experiment.h"
#include "dense_array.h"
#include "utils.h"

#include <string>
#include <vector>
#include <filesystem>
#include <stdexcept>

struct SummarizedExperimentUtilsTest : public ::testing::Test {
    void expect_error_names(const std::filesystem::path& dir, const std::string& msg) {
        EXPECT_ANY_THROW({
            try {
                takane::internal_summarized_experiment::check_names_json(dir);
            } catch (std::exception& e) {
                EXPECT_THAT(e.what(), ::testing::HasSubstr(msg));
                throw;
            }
        });
    }
};

TEST_F(SummarizedExperimentUtilsTest, CheckNames) {
    std::filesystem::path dir = "TEST_se_utils_names";
    std::filesystem::create_directory(dir);

    {
        std::ofstream handle(dir / "names.json");
        handle << "{}";
    }
    expect_error_names(dir, "an array");

    {
        std::ofstream handle(dir / "names.json");
        handle << "[1,2]";
    }
    expect_error_names(dir, "an array of strings");

    {
        std::ofstream handle(dir / "names.json");
        handle << "[\"aaron\",\"aaron\"]";
    }
    expect_error_names(dir, "duplicated name 'aaron'");

    {
        std::ofstream handle(dir / "names.json");
        handle << "[\"aaron\",\"\"]";
    }
    expect_error_names(dir, "empty string");

    {
        std::ofstream handle(dir / "names.json");
        handle << "[\"aaron\",\"charlie\",\"sandman\"]";
    }
    EXPECT_EQ(takane::internal_summarized_experiment::check_names_json(dir), 3);
}
