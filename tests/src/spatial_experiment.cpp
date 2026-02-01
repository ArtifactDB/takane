#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "spatial_experiment.h"
#include "utils.h"

#include "takane/spatial_experiment.hpp"

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
    auto parsed = millijson::parse_file(opath.c_str(), {});
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

TEST_F(SpatialExperimentTest, NoImages) {
    spatial_experiment::Options options(20, 19);
    options.num_samples = 3;
    options.num_images_per_sample = 0;
    spatial_experiment::mock(dir, options);

    std::filesystem::remove_all(dir / "images");
    EXPECT_FALSE(std::filesystem::exists(dir / "images"));

    // Bumping the version to support no images/ subdirectory.
    {
        auto opath = dir / "OBJECT";
        auto parsed = millijson::parse_file(opath.c_str(), {});
        auto& remap = reinterpret_cast<millijson::Object*>(parsed.get())->value();
        remap["type"] = std::shared_ptr<millijson::Base>(new millijson::String("spatial_experiment"));
        spatial_experiment::add_object_metadata(parsed.get(), "1.2");
        json_utils::dump(parsed.get(), opath);
    }

    test_validate(dir); 
}

TEST_F(SpatialExperimentTest, ExtraImages) {
    spatial_experiment::Options options(20, 19);
    options.num_samples = 2;
    options.num_images_per_sample = 1;
    spatial_experiment::mock(dir, options);

    {
        auto ipath = dir / "images" / "2.tif";
        std::ofstream ohandle(ipath);
    }
    expect_error("more objects than expected");
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

TEST_F(SpatialExperimentTest, OtherImageFormats) {
    spatial_experiment::Options options(20, 19);
    options.num_samples = 2;
    options.num_images_per_sample = 7;
    spatial_experiment::mock(dir, options);

    // Bumping the version so we actually get support for OTHER image types.
    {
        auto opath = dir / "OBJECT";
        auto parsed = millijson::parse_file(opath.c_str(), {});
        auto& remap = reinterpret_cast<millijson::Object*>(parsed.get())->value();
        remap["type"] = std::shared_ptr<millijson::Base>(new millijson::String("spatial_experiment"));
        spatial_experiment::add_object_metadata(parsed.get(), "1.1");
        json_utils::dump(parsed.get(), opath);
    }

    size_t num_images = options.num_samples * options.num_images_per_sample;
    {
        H5::H5File handle(dir / "images" / "mapping.h5", H5F_ACC_RDWR);
        auto ghandle = handle.openGroup("spatial_experiment");
        ghandle.unlink("image_formats");
        std::vector<std::string> replacement(num_images, "OTHER");
        hdf5_utils::spawn_string_data(ghandle, "image_formats", H5T_VARIABLE, replacement);
    }

    {
        auto idir = dir / "images";
        for (const auto & entry : std::filesystem::directory_iterator(idir)) {
            if (entry.path().filename() != "mapping.h5") {
                std::filesystem::remove(entry.path());
            }
        }

        for (size_t i = 0; i < num_images; ++i) {
            auto ipath = idir / std::to_string(i);
            std::filesystem::remove(ipath);
            std::filesystem::create_directory(ipath);

            auto optr = new millijson::Object({});
            std::shared_ptr<millijson::Base> contents(optr);
            optr->value()["type"] = std::shared_ptr<millijson::Base>(new millijson::String("some_image_class"));
            optr->value()["version"] = std::shared_ptr<millijson::Base>(new millijson::String("1.0"));
            json_utils::dump(contents.get(), ipath / "OBJECT");
        }
    }
    expect_error("satisfy the 'IMAGE' interface");

    takane::Options vopt;
    vopt.custom_satisfies_interface["IMAGE"].insert("some_image_class");
    EXPECT_ANY_THROW({
        try {
            test_validate(dir, vopt);
        } catch (std::exception& e) {
            EXPECT_THAT(e.what(), ::testing::HasSubstr("no registered"));
            throw;
        }
    });

    // Mocking up a no-op function for our new class.
    vopt.custom_validate["some_image_class"] = [](const std::filesystem::path&, const takane::ObjectMetadata&, takane::Options&) -> void {};
    test_validate(dir, vopt);
}

template<typename ... Args_>
static void expect_image_error(const std::string& msg, Args_&& ... args) {
    EXPECT_ANY_THROW({
        try {
            takane::spatial_experiment::internal::validate_image(std::forward<Args_>(args)...);
        } catch (std::exception& e) {
            EXPECT_THAT(e.what(), ::testing::HasSubstr(msg));
            throw;
        }
    });
}

TEST_F(SpatialExperimentTest, ImageSignatures) {
    takane::Options opt;
    const ritsuko::Version ver(1, 0, 0);

    {
        initialize_directory(dir);
        auto ipath = dir / "0.png";
        {
            std::ofstream ohandle(ipath);
        }
        expect_image_error("incomplete PNG file signature", dir, 0, "PNG", opt, ver);

        {
            std::ofstream ohandle(ipath);
            ohandle << "chino-chan";
        }
        expect_image_error("incorrect PNG file signature", dir, 0, "PNG", opt, ver);

        {
            std::ofstream ohandle(ipath);
            constexpr std::array<unsigned char, 8> stuff { 137, 80, 78, 71, 13, 10, 26, 10 };
            ohandle.write(reinterpret_cast<const char*>(stuff.data()), stuff.size());
        }
        takane::spatial_experiment::internal::validate_image(dir, 0, "PNG", opt, ver);
    }

    {
        initialize_directory(dir);
        auto ipath = dir / "1.tif";
        {
            std::ofstream ohandle(ipath);
        }
        expect_image_error("too small", dir, 1, "TIFF", opt, ver);

        {
            std::ofstream ohandle(ipath);
            ohandle << "chino-chan";
        }
        expect_image_error("incorrect TIFF file signature", dir, 1, "TIFF", opt, ver);

        {
            std::ofstream ohandle(ipath);
            constexpr std::array<unsigned char, 4> stuff{ 0x49, 0x49, 0x2A, 0x00 };
            ohandle.write(reinterpret_cast<const char*>(stuff.data()), stuff.size());
        }
        takane::spatial_experiment::internal::validate_image(dir, 1, "TIFF", opt, ver);

        {
            std::ofstream ohandle(ipath);
            constexpr std::array<unsigned char, 4> stuff{ 0x4D, 0x4D, 0x00, 0x2A };
            ohandle.write(reinterpret_cast<const char*>(stuff.data()), stuff.size());
        }
        takane::spatial_experiment::internal::validate_image(dir, 1, "TIFF", opt, ver);
    }

    const ritsuko::Version latestver(1, 3, 0);

    {
        initialize_directory(dir);
        auto ipath = dir / "2.jpg";
        {
            std::ofstream ohandle(ipath);
        }
        expect_image_error("not currently supported", dir, 2, "JPEG", opt, ver);
        expect_image_error("incomplete JPEG file signature", dir, 2, "JPEG", opt, latestver);

        {
            std::ofstream ohandle(ipath);
            ohandle << "chino-chan";
        }
        expect_image_error("incorrect JPEG file signature", dir, 2, "JPEG", opt, latestver);

        {
            std::ofstream ohandle(ipath);
            constexpr std::array<unsigned char, 4> stuff { 0xff, 0xd8, 0xff, 0xe1 };
            ohandle.write(reinterpret_cast<const char*>(stuff.data()), stuff.size());
        }
        takane::spatial_experiment::internal::validate_image(dir, 2, "JPEG", opt, latestver);
    }

    {
        initialize_directory(dir);
        auto ipath = dir / "3.gif";
        {
            std::ofstream ohandle(ipath);
        }
        expect_image_error("not currently supported", dir, 3, "GIF", opt, ver);
        expect_image_error("incomplete GIF file signature", dir, 3, "GIF", opt, latestver);

        {
            std::ofstream ohandle(ipath);
            ohandle << "chino-chan";
        }
        expect_image_error("incorrect GIF file signature", dir, 3, "GIF", opt, latestver);

        {
            std::ofstream ohandle(ipath);
            constexpr std::array<unsigned char, 4> stuff { 0x47, 0x49, 0x46, 0x38 };
            ohandle.write(reinterpret_cast<const char*>(stuff.data()), stuff.size());
        }
        takane::spatial_experiment::internal::validate_image(dir, 3, "GIF", opt, latestver);
    }

    {
        initialize_directory(dir);
        auto ipath = dir / "4.webp";
        {
            std::ofstream ohandle(ipath);
        }
        expect_image_error("not currently supported", dir, 4, "WEBP", opt, ver);
        expect_image_error("too small", dir, 4, "WEBP", opt, latestver);

        {
            std::ofstream ohandle(ipath);
            ohandle << "kirima-syaro";
        }
        expect_image_error("incorrect WEBP file signature", dir, 4, "WEBP", opt, latestver);

        {
            std::ofstream ohandle(ipath);
            constexpr std::array<unsigned char, 12> stuff { 0x52, 0x49, 0x46, 0x46, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 };
            ohandle.write(reinterpret_cast<const char*>(stuff.data()), stuff.size());
        }
        expect_image_error("incorrect WEBP file signature", dir, 4, "WEBP", opt, latestver);

        {
            std::ofstream ohandle(ipath);
            constexpr std::array<unsigned char, 12> stuff { 0x52, 0x49, 0x46, 0x46, 0x0, 0x0, 0x0, 0x0, 0x57, 0x45, 0x42, 0x50 };
            ohandle.write(reinterpret_cast<const char*>(stuff.data()), stuff.size());
        }
        takane::spatial_experiment::internal::validate_image(dir, 4, "WEBP", opt, latestver);
    }
}
