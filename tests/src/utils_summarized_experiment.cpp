#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "takane/utils_summarized_experiment.hpp"

#include "utils.h"

#include <string>
#include <vector>
#include <filesystem>
#include <stdexcept>
#include <fstream>

TEST(ExtractSummarizedExperimentDimensions, Okay) {
    auto dir = define_test_path("utils_summarized_experiment");
    initialize_directory(dir);
    auto path = dir / "OBJECT";

    {
        quick_text_write(path, "{ \"type\": \"summarized_experiment\", \"summarized_experiment\": { \"dimensions\": [10, 20] } }");
    }
    {
        auto meta = takane::read_object_metadata(dir);
        auto semap = takane::extract_json_object(meta.other, "summarized_experiment");
        EXPECT_EQ(takane::extract_summarized_experiment_dimensions(semap), (std::pair<std::size_t, std::size_t>(10, 20)));
    }

    {
        quick_text_write(path, "{ \"type\": \"summarized_experiment\", \"summarized_experiment\": { \"dimensions\": [0, 0] } }");
    }
    {
        auto meta = takane::read_object_metadata(dir);
        auto semap = takane::extract_json_object(meta.other, "summarized_experiment");
        EXPECT_EQ(takane::extract_summarized_experiment_dimensions(semap), (std::pair<std::size_t, std::size_t>(0, 0)));
    }
}

static void expect_error_dimensions(const std::filesystem::path& dir, const std::string& msg) {
    auto meta = takane::read_object_metadata(dir);
    auto semap = takane::extract_json_object(meta.other, "summarized_experiment");
    expect_error(
        msg,
        [&]() -> void {
            takane::extract_summarized_experiment_dimensions(semap);
        }
    );
}

TEST(ExtractSummarizedExperimentDimensions, Error) {
    auto dir = define_test_path("utils_summarized_experiment");
    initialize_directory(dir);
    auto path = dir / "OBJECT";

    {
        quick_text_write(path, "{ \"type\": \"summarized_experiment\", \"summarized_experiment\": {} }");
    }
    expect_error_dimensions(dir, "expected a 'dimensions'");

    {
        quick_text_write(path, "{ \"type\": \"summarized_experiment\", \"summarized_experiment\": { \"dimensions\": null } }");
    }
    expect_error_dimensions(dir, "to be an array");

    {
        quick_text_write(path, "{ \"type\": \"summarized_experiment\", \"summarized_experiment\": { \"dimensions\": [] } }");
    }
    expect_error_dimensions(dir, "length 2");

    {
        quick_text_write(path, "{ \"type\": \"summarized_experiment\", \"summarized_experiment\": { \"dimensions\": [ \"foo\", \"bar\" ] } }");
    }
    expect_error_dimensions(dir, "array of numbers");

    {
        quick_text_write(path, "{ \"type\": \"summarized_experiment\", \"summarized_experiment\": { \"dimensions\": [ -1, 0 ] } }");
    }
    expect_error_dimensions(dir, "non-negative");

    {
        quick_text_write(path, "{ \"type\": \"summarized_experiment\", \"summarized_experiment\": { \"dimensions\": [ 0, 20.5 ] } }");
    }
    expect_error_dimensions(dir, "integers");
}

/***************************************/

TEST(ExtractSummarizedExperimentNames, Okay) {
    auto dir = define_test_path("utils_summarized_experiment");
    initialize_directory(dir);
    auto path = dir / "names.json";

    {
        quick_text_write(path, "[\"aaron\",\"charlie\",\"sandman\"]");
    }
    EXPECT_EQ(takane::extract_summarized_experiment_names(path), (std::vector<std::string>{ "aaron", "charlie", "sandman" }));

    {
        quick_text_write(path, "[\"foo\",\"bar\"]");
    }
    EXPECT_EQ(takane::extract_summarized_experiment_names(path), (std::vector<std::string>{ "foo", "bar" }));

    {
        quick_text_write(path, "[]");
    }
    EXPECT_EQ(takane::extract_summarized_experiment_names(path), (std::vector<std::string>()));
}

static void expect_error_names(const std::filesystem::path& path, const std::string& msg) {
    expect_error(
        msg,
        [&]() -> void {
            takane::extract_summarized_experiment_names(path);
        }
    );
}

TEST(ExtractSummarizedExperimentNames, Error) {
    auto dir = define_test_path("utils_summarized_experiment");
    initialize_directory(dir);
    auto path = dir / "names.json";

    {
        quick_text_write(path, "{}");
    }
    expect_error_names(path, "an array");

    {
        quick_text_write(path, "[1,2]");
    }
    expect_error_names(path, "an array of strings");

    {
        quick_text_write(path, "[\"aaron\",\"aaron\"]");
    }
    expect_error_names(path, "duplicated name 'aaron'");

    {
        quick_text_write(path, "[\"aaron\",\"\"]");
    }
    expect_error_names(path, "empty string");
}
