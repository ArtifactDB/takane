#include <gtest/gtest.h>
#include <gmock/gmock.h>

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
                test_validate(dir);
            } catch (std::exception& e) {
                EXPECT_THAT(e.what(), ::testing::HasSubstr(msg));
                throw;
            }
        });
    }
};

TEST_F(SummarizedExperimentTest, Metadata) {
    initialize_directory(dir);
    auto dump = [&](const std::string& contents) {
        std::ofstream handle(dir / "OBJECT");
        handle << "{ \"type\": \"" + name + "\", \"" + name + "\": " + contents + " }";
    };

    dump("[]");
    expect_error("should be a JSON object");

    dump("{}");
    expect_error("'summarized_experiment.version'");

    dump("{ \"version\": 1.0 }");
    expect_error("should be a JSON string");

    dump("{ \"version\": \"2.0\" }");
    expect_error("unsupported version");

    dump("{ \"version\": \"1.0\" }");
    expect_error("expected a 'dimensions'");

    dump("{ \"version\": \"1.0\", \"dimensions\": 1 }");
    expect_error("to be an array");

    dump("{ \"version\": \"1.0\", \"dimensions\": [] }");
    expect_error("array of length 2");

    dump("{ \"version\": \"1.0\", \"dimensions\": [ true, false ] }");
    expect_error("array of numbers");

    dump("{ \"version\": \"1.0\", \"dimensions\": [ -1, 1 ] }");
    expect_error("non-negative integers");

    dump("{ \"version\": \"1.0\", \"dimensions\": [ 10, 1.5 ] }");
    expect_error("non-negative integers");

    dump("{ \"version\": \"1.0\", \"dimensions\": [ 10, 20 ] }");
    test_validate(dir); // works without any assays!
}

TEST_F(SummarizedExperimentTest, Assays) {
    summarized_experiment::mock(dir, summarized_experiment::Options(10, 15, 2));
    test_validate(dir); // success!
    EXPECT_EQ(test_height(dir), 10);
    std::vector<size_t> expected_dim{10, 15};
    EXPECT_EQ(test_dimensions(dir), expected_dim);

    {
        // Most checks are handled by utils_summarized_experiment.cpp,
        // we just do a cursory check to ensure that it does get called.
        std::ofstream handle(dir / "assays" / "names.json");
        handle << "[\"aaron\",\"\"]";
    }
    expect_error("empty string");

    {
        std::ofstream handle(dir / "assays" / "names.json");
        handle << "[\"aaron\",\"jayaram\"]";
        simple_list::mock(dir / "assays" / "0");
    }
    expect_error("no registered 'dimensions' function");

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
    summarized_experiment::Options options(10, 15, 2);
    options.has_row_data = true;
    options.has_column_data = true;
    options.has_other_data = true;
    summarized_experiment::mock(dir, options);
    test_validate(dir); // success!

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
