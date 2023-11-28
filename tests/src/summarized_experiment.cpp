#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "takane/takane.hpp"

#include "summarized_experiment.h"
#include "dense_array.h"
#include "utils.h"

#include <string>
#include <vector>
#include <filesystem>
#include <stdexcept>

struct SummarizedExperimentTest : public ::testing::Test {
    SummarizedExperimentTest() {
        dir = "TEST_summarized_experiment";
        name = "summarized_experiment";
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

TEST_F(SummarizedExperimentTest, TopLevelJson) {
    {
        initialize_directory(dir, name);
        std::ofstream handle(dir / "summarized_experiment.json");
        handle << "[]";
    }
    expect_error("top-level object");

    {
        initialize_directory(dir, name);
        std::ofstream handle(dir / "summarized_experiment.json");
        handle << "{}";
    }
    expect_error("expected a 'version'");

    {
        std::ofstream handle(dir / "summarized_experiment.json");
        handle << "{ \"version\": 1.0 }";
    }
    expect_error("to be a string");

    {
        std::ofstream handle(dir / "summarized_experiment.json");
        handle << "{ \"version\": \"2.0\" }";
    }
    expect_error("unsupported version");

    {
        std::ofstream handle(dir / "summarized_experiment.json");
        handle << "{ \"version\": \"1.0\" }";
    }
    expect_error("expected a 'dimensions'");

    {
        std::ofstream handle(dir / "summarized_experiment.json");
        handle << "{ \"version\": \"1.0\", \"dimensions\": 1 }";
    }
    expect_error("to be an array");

    {
        std::ofstream handle(dir / "summarized_experiment.json");
        handle << "{ \"version\": \"1.0\", \"dimensions\": [] }";
    }
    expect_error("array of length 2");

    {
        std::ofstream handle(dir / "summarized_experiment.json");
        handle << "{ \"version\": \"1.0\", \"dimensions\": [ true, false ] }";
    }
    expect_error("array of numbers");

    {
        std::ofstream handle(dir / "summarized_experiment.json");
        handle << "{ \"version\": \"1.0\", \"dimensions\": [ -1, 1 ] }";
    }
    expect_error("non-negative integers");

    {
        std::ofstream handle(dir / "summarized_experiment.json");
        handle << "{ \"version\": \"1.0\", \"dimensions\": [ 10, 1.5 ] }";
    }
    expect_error("non-negative integers");
}

TEST_F(SummarizedExperimentTest, Assays) {
    summarized_experiment::mock(dir, 10, 15, 2);
    takane::validate(dir); // success!
    EXPECT_EQ(takane::height(dir), 10);
    std::vector<size_t> expected_dim{10, 15};
    EXPECT_EQ(takane::dimensions(dir), expected_dim);

    {
        std::ofstream handle(dir / "assays" / "names.json");
        handle << "{}";
    }
    expect_error("an array");

    {
        std::ofstream handle(dir / "assays" / "names.json");
        handle << "[1,2]";
    }
    expect_error("an array of strings");

    {
        std::ofstream handle(dir / "assays" / "names.json");
        handle << "[\"aaron\",\"aaron\"]";
    }
    expect_error("duplicated assay name 'aaron'");

    {
        std::ofstream handle(dir / "assays" / "names.json");
        handle << "[\"aaron\",\"jayaram\"]";
        simple_list::mock(dir / "assays" / "0");
    }
    expect_error("no registered 'dimensions' method");

    {
        dense_array::mock(dir / "assays" / "0", dense_array::Type::INTEGER, { 10 });
    }
    expect_error("two or more dimensions");

    {
        dense_array::mock(dir / "assays" / "0", dense_array::Type::INTEGER, { 20, 15 });
    }
    expect_error("number of rows");

    {
        dense_array::mock(dir / "assays" / "0", dense_array::Type::INTEGER, { 10, 20 });
    }
    expect_error("number of columns");

    {
        dense_array::mock(dir / "assays" / "0", dense_array::Type::INTEGER, { 10, 15 });
        dense_array::mock(dir / "assays" / "foobar", dense_array::Type::INTEGER, { 10, 15 });
    }
    expect_error("more objects than expected");
}


TEST_F(SummarizedExperimentTest, AllOtherData) {
    summarized_experiment::mock(dir, 10, 15, 2, true, true, true);
    takane::validate(dir); // success!

    {
        simple_list::mock(dir / "column_data");
    }
    expect_error("object in 'column_data' should satisfy");

    {
        data_frame::mock(dir / "column_data", 10, {});
    }
    expect_error("number of columns");

    {
        data_frame::mock(dir / "column_data", 15, {});
        simple_list::mock(dir / "row_data");
    }
    expect_error("object in 'row_data' should satisfy");

    {
        data_frame::mock(dir / "row_data", 20, {});
    }
    expect_error("number of rows");

    {
        data_frame::mock(dir / "row_data", 10, {});
        dense_array::mock(dir / "other_data", dense_array::Type::INTEGER, { 10, 20 });
    }
    expect_error("'SIMPLE_LIST' interface");
}
