#include <gtest/gtest.h>
#include <gmock/gmock.h>

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
                test_validate(dir);
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
    test_validate(dir); 
    EXPECT_EQ(test_height(dir), 20);
    std::vector<size_t> expected_dim{ 20, 15 };
    EXPECT_EQ(test_dimensions(dir), expected_dim);

    // Mocking up more samples and images per sample.
    options.num_samples = 4;
    options.num_images_per_sample = 3;
    spatial_experiment::mock(dir, options);
    test_validate(dir); 
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

TEST_F(SpatialExperimentTest, ColumnSamples) {
    spatial_experiment::Options options(20, 19);
    options.num_samples = 3;
    spatial_experiment::mock(dir, options);
    test_validate(dir); 

    {
        H5::H5File handle(dir / "images" / "mapping.h5", H5F_ACC_RDWR);
        auto ghandle = handle.openGroup("spatial_experiment");
        ghandle.unlink("sample_names");

        std::vector<std::string> duplicated { "Aaron", "Aaron", "Jayaram" };
        hdf5_utils::spawn_string_data(ghandle, "sample_names", H5T_VARIABLE, duplicated);
    }
    expect_error("duplicated sample name");

    // There should be three samples, but we'll reduce it down to 2.
    {
        H5::H5File handle(dir / "images" / "mapping.h5", H5F_ACC_RDWR);
        auto ghandle = handle.openGroup("spatial_experiment");
        ghandle.unlink("sample_names");
        std::vector<std::string> restored { "Aaron", "Jayaram" };
        hdf5_utils::spawn_string_data(ghandle, "sample_names", H5T_VARIABLE, restored);
    }
    expect_error("less than the number of sample names");

    {
        H5::H5File handle(dir / "images" / "mapping.h5", H5F_ACC_RDWR);
        auto ghandle = handle.openGroup("spatial_experiment");
        ghandle.unlink("column_samples");
        std::vector<int> assignments(20);
        hdf5_utils::spawn_numeric_data(ghandle, "column_samples", H5::PredType::NATIVE_UINT8, assignments);
    }
    expect_error("equal the number of columns");
}

TEST_F(SpatialExperimentTest, ImageSamples) {
    spatial_experiment::Options options(20, 19);
    options.num_samples = 3;
    options.num_images_per_sample = 2;
    spatial_experiment::mock(dir, options);
    test_validate(dir); 

    {
        H5::H5File handle(dir / "images" / "mapping.h5", H5F_ACC_RDWR);
        auto ghandle = handle.openGroup("spatial_experiment");
        ghandle.unlink("image_samples");
        std::vector<int> replacement(options.num_samples * options.num_images_per_sample);
        hdf5_utils::spawn_numeric_data(ghandle, "image_samples", H5::PredType::NATIVE_INT32, replacement);
    }
    expect_error("64-bit unsigned integer");

    {
        H5::H5File handle(dir / "images" / "mapping.h5", H5F_ACC_RDWR);
        auto ghandle = handle.openGroup("spatial_experiment");
        ghandle.unlink("image_samples");
        std::vector<int> replacement(options.num_samples * options.num_images_per_sample, 100);
        hdf5_utils::spawn_numeric_data(ghandle, "image_samples", H5::PredType::NATIVE_UINT8, replacement);
    }
    expect_error("less than the number of samples");

    // Adding an extra sample that doesn't have anything mapping to it.
    spatial_experiment::mock(dir, options);
    {
        H5::H5File handle(dir / "images" / "mapping.h5", H5F_ACC_RDWR);
        auto ghandle = handle.openGroup("spatial_experiment");
        ghandle.unlink("sample_names");
        std::vector<std::string> replacement{ "chinatsu", "akane", "makoto", "inukai" };
        hdf5_utils::spawn_string_data(ghandle, "sample_names", H5T_VARIABLE, replacement);
    }
    expect_error("map to one or more images");
}

TEST_F(SpatialExperimentTest, ImageIds) {
    spatial_experiment::Options options(20, 19);
    options.num_samples = 2;
    options.num_images_per_sample = 3;
    spatial_experiment::mock(dir, options);
    test_validate(dir); 

    {
        H5::H5File handle(dir / "images" / "mapping.h5", H5F_ACC_RDWR);
        auto ghandle = handle.openGroup("spatial_experiment");
        ghandle.unlink("image_ids");
        hdf5_utils::spawn_numeric_data<int>(ghandle, "image_ids", H5::PredType::NATIVE_INT, {});
    }
    expect_error("represented by a UTF-8 encoded string");

    {
        H5::H5File handle(dir / "images" / "mapping.h5", H5F_ACC_RDWR);
        auto ghandle = handle.openGroup("spatial_experiment");
        ghandle.unlink("image_ids");
        hdf5_utils::spawn_string_data(ghandle, "image_ids", H5T_VARIABLE, {});
    }
    expect_error("same length as 'image_samples'");

    // All image IDs are now empty strings.
    {
        H5::H5File handle(dir / "images" / "mapping.h5", H5F_ACC_RDWR);
        auto ghandle = handle.openGroup("spatial_experiment");
        ghandle.unlink("image_ids");
        std::vector<std::string> replacement(options.num_samples * options.num_images_per_sample);
        hdf5_utils::spawn_string_data(ghandle, "image_ids", 1, replacement);
    }
    expect_error("contains duplicated image IDs");
}

TEST_F(SpatialExperimentTest, ImageScaling) {
    spatial_experiment::Options options(20, 19);
    options.num_samples = 1;
    options.num_images_per_sample = 4;
    spatial_experiment::mock(dir, options);
    test_validate(dir); 

    {
        H5::H5File handle(dir / "images" / "mapping.h5", H5F_ACC_RDWR);
        auto ghandle = handle.openGroup("spatial_experiment");
        ghandle.unlink("image_scale_factors");
        hdf5_utils::spawn_string_data(ghandle, "image_scale_factors", H5T_VARIABLE, {});
    }
    expect_error("64-bit float");

    {
        H5::H5File handle(dir / "images" / "mapping.h5", H5F_ACC_RDWR);
        auto ghandle = handle.openGroup("spatial_experiment");
        ghandle.unlink("image_scale_factors");
        hdf5_utils::spawn_numeric_data<double>(ghandle, "image_scale_factors", H5::PredType::NATIVE_DOUBLE, {});
    }
    expect_error("same length as 'image_samples'");

    {
        H5::H5File handle(dir / "images" / "mapping.h5", H5F_ACC_RDWR);
        auto ghandle = handle.openGroup("spatial_experiment");
        ghandle.unlink("image_scale_factors");
        hdf5_utils::spawn_numeric_data<double>(ghandle, "image_scale_factors", H5::PredType::NATIVE_DOUBLE, std::vector<double>(4));
    }
    expect_error("finite and positive");

    {
        H5::H5File handle(dir / "images" / "mapping.h5", H5F_ACC_RDWR);
        auto ghandle = handle.openGroup("spatial_experiment");
        ghandle.unlink("image_scale_factors");
        std::vector<double> replacement(4, std::numeric_limits<double>::quiet_NaN());
        hdf5_utils::spawn_numeric_data<double>(ghandle, "image_scale_factors", H5::PredType::NATIVE_DOUBLE, replacement);
    }
    expect_error("finite and positive");
}

TEST_F(SpatialExperimentTest, ImageFormats) {
    spatial_experiment::Options options(20, 19);
    options.num_samples = 2;
    options.num_images_per_sample = 7;
    spatial_experiment::mock(dir, options);
    test_validate(dir); 

    {
        H5::H5File handle(dir / "images" / "mapping.h5", H5F_ACC_RDWR);
        auto ghandle = handle.openGroup("spatial_experiment");
        ghandle.unlink("image_formats");
        hdf5_utils::spawn_numeric_data<double>(ghandle, "image_formats", H5::PredType::NATIVE_DOUBLE, {});
    }
    expect_error("represented by a UTF-8 encoded string");

    {
        H5::H5File handle(dir / "images" / "mapping.h5", H5F_ACC_RDWR);
        auto ghandle = handle.openGroup("spatial_experiment");
        ghandle.unlink("image_formats");
        hdf5_utils::spawn_string_data(ghandle, "image_formats", H5T_VARIABLE, {});
    }
    expect_error("same length as 'image_samples'");

    {
        H5::H5File handle(dir / "images" / "mapping.h5", H5F_ACC_RDWR);
        auto ghandle = handle.openGroup("spatial_experiment");
        ghandle.unlink("image_formats");
        std::vector<std::string> replacement(options.num_samples * options.num_images_per_sample, "FOOBAR");
        hdf5_utils::spawn_string_data(ghandle, "image_formats", H5T_VARIABLE, replacement);
    }
    expect_error("not currently supported");
}

TEST_F(SpatialExperimentTest, ImageSignature) {
    spatial_experiment::Options options(20, 19);
    options.num_samples = 1;
    options.num_images_per_sample = 2;
    spatial_experiment::mock(dir, options);
    test_validate(dir); 

    // Overriding each image.
    {
        auto ipath = dir / "images" / "0.png";
        {
            std::ofstream ohandle(ipath);
        }
        expect_error("incomplete PNG file signature");

        {
            std::ofstream ohandle(ipath);
            ohandle << "chino-chan";
        }
        expect_error("incorrect PNG file signature");
    }

    spatial_experiment::mock(dir, options);
    {
        auto ipath = dir / "images" / "1.tif";
        {
            std::ofstream ohandle(ipath);
        }
        expect_error("too small");

        {
            std::ofstream ohandle(ipath);
            ohandle << "chino-chan";
        }
        expect_error("incorrect TIFF file signature");
    }

    // Replacing an image.
    spatial_experiment::mock(dir, options);
    {
        auto ipath = dir / "images" / "2.tif";
        std::ofstream ohandle(ipath);
    }
    expect_error("more objects than expected");
}
