#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "takane/takane.hpp"

#include "spatial_experiment.h"
#include "utils.h"

#include <string>
#include <vector>
#include <filesystem>
#include <stdexcept>

struct SpatialExperimentTest : public ::testing::Test {
    SpatialExperimentTest() {
        dir = "TEST_spatial_experiment";
        name = "spatial_experiment";
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

TEST_F(SpatialExperimentTest, Basic) {
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

    // Hits the base SCE checks.
    {
        ::ranged_summarized_experiment::add_object_metadata(parsed.get(), "1.0");
        json_utils::dump(parsed.get(), opath);
    }
    expect_error("'single_cell_experiment'");

    // Hits that spatial metadata is recognized.
    {
        ::single_cell_experiment::add_object_metadata(parsed.get(), "1.0", "whee");
        ::spatial_experiment::add_object_metadata(parsed.get(), "2.0");
        json_utils::dump(parsed.get(), opath);
    }
    expect_error("unsupported version");

    // Success!
    spatial_experiment::Options options(20, 15);
    spatial_experiment::mock(dir, options);
    takane::validate(dir); 
    EXPECT_EQ(takane::height(dir), 20);
    std::vector<size_t> expected_dim{ 20, 15 };
    EXPECT_EQ(takane::dimensions(dir), expected_dim);

    // Mocking up more samples and images per sample.
    options.num_samples = 4;
    options.num_images_per_sample = 3;
    spatial_experiment::mock(dir, options);
    takane::validate(dir); 
}

TEST_F(SpatialExperimentTest, Coordinates) {
    spatial_experiment::Options options(20, 19);
    spatial_experiment::mock(dir, options);

    simple_list::mock(dir / "coordinates");
    expect_error("should be a dense array");

    initialize_directory_simple(dir / "coordinates", "dense_array", "1.0");
    expect_error("failed to validate 'coordinates'");

    std::vector<hsize_t> dims(1);
    dims[0] = options.num_cols;
    dense_array::mock(dir / "coordinates", dense_array::Type::NUMBER, dims);
    expect_error("2-dimensional dense array");

    dims.push_back(4);
    dense_array::mock(dir / "coordinates", dense_array::Type::NUMBER, dims);
    expect_error("2 or 3 columns");

    dims.back() = 2;
    dims.front() += 1;
    dense_array::mock(dir / "coordinates", dense_array::Type::NUMBER, dims);
    expect_error("should equal the number of columns");

    dims.front() = options.num_cols;
    dense_array::mock(dir / "coordinates", dense_array::Type::STRING, dims);
    expect_error("should be numeric");
}
