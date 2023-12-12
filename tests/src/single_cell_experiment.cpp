#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "takane/takane.hpp"

#include "single_cell_experiment.h"
#include "utils.h"

#include <string>
#include <vector>
#include <filesystem>
#include <stdexcept>

struct SingleCellExperimentTest : public ::testing::Test {
    SingleCellExperimentTest() {
        dir = "TEST_single_cell_experiment";
        name = "single_cell_experiment";
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

TEST_F(SingleCellExperimentTest, Basic) {
    // Hits the base SE checks.
    initialize_directory_simple(dir, name, "1.0");
    expect_error("'summarized_experiment'");

    // Hits the base RSE checks.
    auto opath = dir / "OBJECT";
    auto parsed = millijson::parse_file(opath.c_str());
    {
        ::summarized_experiment::add_object_metadata(parsed.get(), "1.0", 99, 23);
        json_utils::dump(parsed.get(), opath);
    }
    expect_error("'ranged_summarized_experiment'");

    // Check that SCE metadata is recognized.
    {
        ::ranged_summarized_experiment::add_object_metadata(parsed.get(), "1.0");
        ::single_cell_experiment::add_object_metadata(parsed.get(), "2.0", "");
        json_utils::dump(parsed.get(), opath);
    }
    expect_error("unsupported version");

    single_cell_experiment::Options options(20, 15);
    single_cell_experiment::mock(dir, options);
    takane::validate(dir); 
    EXPECT_EQ(takane::height(dir), 20);
    std::vector<size_t> expected_dim{ 20, 15 };
    EXPECT_EQ(takane::dimensions(dir), expected_dim);
}

TEST_F(SingleCellExperimentTest, ReducedDims) {
    single_cell_experiment::Options options(20, 15);
    options.num_reduced_dims = 2;
    options.num_alt_exps = 0;
    single_cell_experiment::mock(dir, options);

    {
        simple_list::mock(dir / "reduced_dimensions" / "0");
    }
    expect_error("no registered 'dimensions' function");

    {
        dense_array::mock(dir / "reduced_dimensions" / "0", dense_array::Type::INTEGER, {});
    }
    expect_error("at least one dimension");

    {
        dense_array::mock(dir / "reduced_dimensions" / "0", dense_array::Type::INTEGER, { 20, 10 });
    }
    expect_error("number of rows");

    {
        dense_array::mock(dir / "reduced_dimensions" / "0", dense_array::Type::INTEGER, { 15, 5 });
        dense_array::mock(dir / "reduced_dimensions" / "foobar", dense_array::Type::INTEGER, { 20, 10 });
    }
    expect_error("more objects than expected");

    // Absence of reduced_dimensions is allowed.
    {
        std::filesystem::remove_all(dir / "reduced_dimensions");
    }
    takane::validate(dir); 
}

TEST_F(SingleCellExperimentTest, AlternativeExps) {
    single_cell_experiment::Options options(100, 20);
    options.num_reduced_dims = 0;
    options.num_alt_exps = 2;
    single_cell_experiment::mock(dir, options);

    {
        dense_array::mock(dir / "alternative_experiments" / "0", dense_array::Type::INTEGER, { 100, 20 });
    }
    expect_error("'SUMMARIZED_EXPERIMENT' interface");

    {
        summarized_experiment::mock(dir / "alternative_experiments" / "0", summarized_experiment::Options(10, 100));
    }
    expect_error("same number of columns");

    {
        summarized_experiment::mock(dir / "alternative_experiments" / "0", summarized_experiment::Options(10, 20));
        summarized_experiment::mock(dir / "alternative_experiments" / "foobar", summarized_experiment::Options(10, 5));
    }
    expect_error("more objects than expected");

    // Absence of alternative_experiments is allowed.
    {
        std::filesystem::remove_all(dir / "alternative_experiments");
    }
    takane::validate(dir); 
}

TEST_F(SingleCellExperimentTest, MainExperimentName) {
    single_cell_experiment::Options options(100, 20);
    options.num_reduced_dims = 2;
    options.num_alt_exps = 2;
    single_cell_experiment::mock(dir, options);

    auto opath = dir / "OBJECT";
    auto parsed = millijson::parse_file(opath.c_str());
    auto& toplevel = reinterpret_cast<millijson::Object*>(parsed.get())->values;
    auto& scemap = reinterpret_cast<millijson::Object*>(toplevel["single_cell_experiment"].get())->values;
    {
        scemap["main_experiment_name"] = std::shared_ptr<millijson::Base>(new millijson::Number(2));
        json_utils::dump(parsed.get(), opath);
    }
    expect_error("to be a string");

    {
        scemap["main_experiment_name"] = std::shared_ptr<millijson::Base>(new millijson::String(""));
        json_utils::dump(parsed.get(), opath);
    }
    expect_error("empty string");

    {
        scemap["main_experiment_name"] = std::shared_ptr<millijson::Base>(new millijson::String("foo"));
        json_utils::dump(parsed.get(), opath);
        std::ofstream ahandle(dir / "alternative_experiments" / "names.json");
        ahandle << "[ \"foo\", \"bar\" ]";
    }
    expect_error("not overlap");

    // Finally success.
    options.main_exp_name = "stuff";
    single_cell_experiment::mock(dir, options);
    takane::validate(dir);
}
