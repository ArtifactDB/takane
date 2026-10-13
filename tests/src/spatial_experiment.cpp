#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "mock_spatial_experiment.h"
#include "utils.h"

#include <string>
#include <vector>
#include <filesystem>
#include <stdexcept>

TEST(SpatialExperiment, Okay) {
    auto dir = define_test_path("spatial_experiment");

    // One image, one sample.
    {
        mock_spatial_experiment(dir, SpatialExperimentOptions(12, 23));
    }
    {
        test_validate(dir);
        EXPECT_EQ(test_height(dir), 12);
        EXPECT_EQ(test_dimensions(dir), (std::vector<std::size_t>{ 12, 23 }));
    }

    // One image for each of multiple samples.
    {
        SpatialExperimentOptions opt(21, 13);
        opt.num_samples = 4;
        mock_spatial_experiment(dir, opt);
    }
    {
        test_validate(dir);
        EXPECT_EQ(test_height(dir), 21);
        EXPECT_EQ(test_dimensions(dir), (std::vector<std::size_t>{ 21, 13 }));
    }

    // Multiple images for one sample.
    {
        SpatialExperimentOptions opt(15, 17);
        opt.num_images_per_sample = 3;
        mock_spatial_experiment(dir, opt);
    }
    {
        test_validate(dir);
        EXPECT_EQ(test_height(dir), 15);
        EXPECT_EQ(test_dimensions(dir), (std::vector<std::size_t>{ 15, 17 }));
    }

    // Multiple images for multiple samples.
    {
        SpatialExperimentOptions opt(11, 32);
        opt.num_samples = 5;
        opt.num_images_per_sample = 3;
        mock_spatial_experiment(dir, opt);
    }
    {
        test_validate(dir);
        EXPECT_EQ(test_height(dir), 11);
        EXPECT_EQ(test_dimensions(dir), (std::vector<std::size_t>{ 11, 32 }));
    }
}

/******************************************/

TEST(SpatialExperiment, BaseError) {
    auto dir = define_test_path("spatial_experiment");

    {
        SpatialExperimentOptions opt(7, 5);
        mock_spatial_experiment(dir, opt);
        quick_text_write(dir / "reduced_dimensions" / "names.json", "[ true ]");
    }
    expect_validation_error(dir, "array of strings");
}

TEST(SpatialExperiment, VersionError) {
    auto dir = define_test_path("spatial_experiment");

    {
        SpatialExperimentOptions opt(7, 5);
        opt.version = "2.0";
        mock_spatial_experiment(dir, opt);
    }
    expect_validation_error(dir, "unsupported version");
}

/******************************************/

TEST(SpatialExperiment, CoordinatesOkay) {
    auto dir = define_test_path("spatial_experiment");

    // Trying a 3D array. 
    {
        SpatialExperimentOptions opt(19, 23);
        opt.coordinate_dimensions = 3;
        mock_spatial_experiment(dir, opt);
    }
    {
        test_validate(dir);
        EXPECT_EQ(test_height(dir), 19);
        EXPECT_EQ(test_dimensions(dir), (std::vector<std::size_t>{ 19, 23 }));
    }

    // Trying an integer array. 
    {
        mock_spatial_experiment(dir, SpatialExperimentOptions(19, 23));
        std::filesystem::remove_all(dir / "coordinates");
        mock_dense_array(dir / "coordinates", DenseArrayOptions({ 23, 2 }, DenseArrayType::INTEGER));
    }
    {
        test_validate(dir);
        EXPECT_EQ(test_height(dir), 19);
        EXPECT_EQ(test_dimensions(dir), (std::vector<std::size_t>{ 19, 23 }));
    }
}

TEST(SpatialExperiment, CoordinatesError) {
    auto dir = define_test_path("spatial_experiment");

    {
        mock_spatial_experiment(dir, SpatialExperimentOptions(7, 5));
        std::filesystem::remove_all(dir / "coordinates");
        mock_data_frame(dir / "coordinates", DataFrameOptions(5, {}));
    }
    expect_validation_error(dir, "dense array");

    // Actually validates the array itself.
    {
        mock_spatial_experiment(dir, SpatialExperimentOptions(7, 5));
        H5::H5File handle(dir / "coordinates" / "array.h5", H5F_ACC_RDWR);
        auto ghandle = handle.openGroup("dense_array");
        ghandle.removeAttr("type");
        add_hdf5_string_attribute(ghandle, "type", "FOOBAR");
    }
    expect_validation_error(dir, "FOOBAR");

    {
        mock_spatial_experiment(dir, SpatialExperimentOptions(7, 5));
        std::filesystem::remove_all(dir / "coordinates");
        mock_dense_array(dir / "coordinates", DenseArrayOptions({ 5, 2, 1 }, DenseArrayType::INTEGER));
    }
    expect_validation_error(dir, "2-dimensional dense array");

    {
        mock_spatial_experiment(dir, SpatialExperimentOptions(7, 5));
        std::filesystem::remove_all(dir / "coordinates");
        mock_dense_array(dir / "coordinates", DenseArrayOptions({ 5, 4 }, DenseArrayType::INTEGER));
    }
    expect_validation_error(dir, "2 or 3 columns");

    {
        mock_spatial_experiment(dir, SpatialExperimentOptions(7, 5));
        std::filesystem::remove_all(dir / "coordinates");
        mock_dense_array(dir / "coordinates", DenseArrayOptions({ 4, 2 }, DenseArrayType::INTEGER));
    }
    expect_validation_error(dir, "number of columns");

    {
        mock_spatial_experiment(dir, SpatialExperimentOptions(7, 5));
        std::filesystem::remove_all(dir / "coordinates");
        mock_dense_array(dir / "coordinates", DenseArrayOptions({ 5, 2 }, DenseArrayType::STRING));
    }
    expect_validation_error(dir, "numeric");
}

/******************************************/

TEST(SpatialExperiment, NoImagesOkay) {
    auto dir = define_test_path("spatial_experiment");

    {
        SpatialExperimentOptions opt(94, 2);
        opt.version = "1.2";
        mock_spatial_experiment(dir, opt);
        std::filesystem::remove_all(dir / "images");
    }

    test_validate(dir);
    EXPECT_EQ(test_height(dir), 94);
    EXPECT_EQ(test_dimensions(dir), (std::vector<std::size_t>{ 94, 2 }));
}

TEST(SpatialExperiment, NoImagesError) {
    auto dir = define_test_path("spatial_experiment");

    {
        SpatialExperimentOptions opt(94, 2);
        opt.version = "1.1";
        mock_spatial_experiment(dir, opt);
        std::filesystem::remove_all(dir / "images");
    }

    H5Eset_auto(H5E_DEFAULT, NULL, NULL);
    expect_validation_error(dir, "failed to validate 'mapping.h5'");
}

/******************************************/

TEST(SpatialExperiment, MappingSampleNamesError) {
    auto dir = define_test_path("spatial_experiment");

    // Check that sample names are validated via the factor levels utility.
    {
        SpatialExperimentOptions opt(7, 5);
        opt.num_samples = 3;
        auto ghandle = mock_spatial_experiment(dir, opt);
        ghandle.unlink("sample_names");
        std::vector<std::string> sample_names{ "akari", "aika", "akari" };
        add_hdf5_string_dataset(ghandle, "sample_names", sample_names);
    }
    expect_validation_error(dir, "duplicated");
}

TEST(SpatialExperiment, MappingColumnSamplesError) {
    auto dir = define_test_path("spatial_experiment");

    SpatialExperimentOptions opt(7, 5);
    opt.num_samples = 2;

    // Check that column sample codes are validated via the factor codes utility.
    {
        auto ghandle = mock_spatial_experiment(dir, opt);
        ghandle.unlink("column_samples");
        std::vector<int> codes{ 0, 1, 2, 1, 0 };
        add_hdf5_numeric_dataset(ghandle, "column_samples", H5::PredType::NATIVE_UINT8, codes);
    }
    expect_validation_error(dir, "less than the number of sample names");

    {
        auto ghandle = mock_spatial_experiment(dir, opt);
        ghandle.unlink("column_samples");
        std::vector<int> codes(4, 0);
        add_hdf5_numeric_dataset(ghandle, "column_samples", H5::PredType::NATIVE_UINT8, codes);
    }
    expect_validation_error(dir, "number of columns");
}

TEST(SpatialExperiment, MappingColumnSamplesMissingOkay) {
    auto dir = define_test_path("spatial_experiment");

    // Check that missing values are allowed in the column sample codes, effectively "rescuing" the error case.
    // This allows the SpE to store data for SE-level samples that might not match to a image-level sample. 
    {
        SpatialExperimentOptions opt(7, 5);
        opt.num_samples = 2;
        auto ghandle = mock_spatial_experiment(dir, opt);
        ghandle.unlink("column_samples");
        std::vector<int> codes{ 0, 1, 2, 1, 0 };
        auto dhandle = add_hdf5_numeric_dataset(ghandle, "column_samples", H5::PredType::NATIVE_UINT8, codes);
        add_hdf5_numeric_attribute(dhandle, "missing-value-placeholder", H5::PredType::NATIVE_UINT8, 2);
    }

    test_validate(dir);
    EXPECT_EQ(test_height(dir), 7);
    EXPECT_EQ(test_dimensions(dir), (std::vector<std::size_t>{7, 5}));
}

TEST(SpatialExperiment, MappingImageSamplesError) {
    auto dir = define_test_path("spatial_experiment");

    SpatialExperimentOptions opt(7, 5);
    opt.num_samples = 3;
    opt.num_images_per_sample = 2;

    {
        auto ghandle = mock_spatial_experiment(dir, opt);
        ghandle.unlink("image_samples");
        ghandle.createDataSet("image_samples", H5::PredType::NATIVE_FLOAT, create_hdf5_dataspace(6));
    }
    expect_validation_error(dir, "64-bit unsigned integer");

    {
        auto ghandle = mock_spatial_experiment(dir, opt);
        ghandle.unlink("image_samples");
        ghandle.createDataSet("image_samples", H5::PredType::NATIVE_UINT64, H5S_SCALAR);
    }
    expect_validation_error(dir, "1-dimensional");

    {
        auto ghandle = mock_spatial_experiment(dir, opt);
        ghandle.unlink("image_samples");
        std::vector<int> codes(6, opt.num_samples);
        add_hdf5_numeric_dataset(ghandle, "image_samples", H5::PredType::NATIVE_UINT8, codes);
    }
    expect_validation_error(dir, "less than the number of samples");

    {
        auto ghandle = mock_spatial_experiment(dir, opt);
        ghandle.unlink("image_samples");
        std::vector<int> codes(6, 1);
        add_hdf5_numeric_dataset(ghandle, "image_samples", H5::PredType::NATIVE_UINT8, codes);
    }
    expect_validation_error(dir, "at least one image");
}

TEST(SpatialExperiment, MappingImageIdsError) {
    auto dir = define_test_path("spatial_experiment");

    SpatialExperimentOptions opt(7, 5);
    opt.num_samples = 2;
    opt.num_images_per_sample = 3;

    {
        auto ghandle = mock_spatial_experiment(dir, opt);
        ghandle.unlink("image_ids");
        ghandle.createDataSet("image_ids", H5::PredType::NATIVE_FLOAT, create_hdf5_dataspace(6));
    }
    expect_validation_error(dir, "UTF-8 encoded string");

    {
        auto ghandle = mock_spatial_experiment(dir, opt);
        ghandle.unlink("image_ids");
        ghandle.createDataSet("image_ids", H5::StrType(0, 10), H5S_SCALAR);
    }
    expect_validation_error(dir, "1-dimensional");

    {
        auto ghandle = mock_spatial_experiment(dir, opt);
        ghandle.unlink("image_ids");
        ghandle.createDataSet("image_ids", H5::StrType(0, 10), create_hdf5_dataspace(5));
    }
    expect_validation_error(dir, "same as that of 'image_samples'");

    // Multiple checks for duplicated (sample, ID) pairs.
    {
        auto ghandle = mock_spatial_experiment(dir, opt);
        ghandle.unlink("image_samples");
        std::vector<int> codes{ 1, 0, 1, 0, 1, 0 };
        add_hdf5_numeric_dataset(ghandle, "image_samples", H5::PredType::NATIVE_UINT8, codes);
        ghandle.unlink("image_ids");
        std::vector<std::string> ids{ "foo", "bar", "bar", "foo", "foo", "bar" };
        add_hdf5_string_dataset(ghandle, "image_ids", ids);
    }
    expect_validation_error(dir, "duplicated");

    {
        SpatialExperimentOptions opt(7, 5);
        opt.num_samples = 3;
        opt.num_images_per_sample = 3;
        auto ghandle = mock_spatial_experiment(dir, opt);
        ghandle.unlink("image_samples");
        std::vector<int> codes{ 2, 2, 2, 1, 1, 1, 0, 0, 0 };
        add_hdf5_numeric_dataset(ghandle, "image_samples", H5::PredType::NATIVE_UINT8, codes);
        ghandle.unlink("image_ids");
        std::vector<std::string> ids{ "bar", "foo", "stuff", "stuff", "foo", "stuff", "stuff", "bar", "foo" };
        add_hdf5_string_dataset(ghandle, "image_ids", ids);
    }
    expect_validation_error(dir, "duplicated");
}

TEST(SpatialExperiment, MappingImageScaleFactorsError) {
    auto dir = define_test_path("spatial_experiment");

    SpatialExperimentOptions opt(7, 5);
    opt.num_samples = 2;
    opt.num_images_per_sample = 3;

    {
        auto ghandle = mock_spatial_experiment(dir, opt);
        ghandle.unlink("image_scale_factors");
        ghandle.createDataSet("image_scale_factors", H5::PredType::NATIVE_UINT64, create_hdf5_dataspace(6));
    }
    expect_validation_error(dir, "64-bit float");

    {
        auto ghandle = mock_spatial_experiment(dir, opt);
        ghandle.unlink("image_scale_factors");
        ghandle.createDataSet("image_scale_factors", H5::PredType::NATIVE_DOUBLE, H5S_SCALAR);
    }
    expect_validation_error(dir, "1-dimensional");

    {
        auto ghandle = mock_spatial_experiment(dir, opt);
        ghandle.unlink("image_scale_factors");
        ghandle.createDataSet("image_scale_factors", H5::PredType::NATIVE_DOUBLE, create_hdf5_dataspace(7));
    }
    expect_validation_error(dir, "same as that of");

    {
        auto ghandle = mock_spatial_experiment(dir, opt);
        ghandle.unlink("image_scale_factors");
        std::vector<double> sc{ 0.1, 0.0, 2, 3, 0.4, 0.5 };
        add_hdf5_numeric_dataset(ghandle, "image_scale_factors", H5::PredType::NATIVE_DOUBLE, sc);
    }
    expect_validation_error(dir, "positive");

    {
        auto ghandle = mock_spatial_experiment(dir, opt);
        ghandle.unlink("image_scale_factors");
        std::vector<double> sc{ 0.1, 0.2, std::numeric_limits<double>::infinity(), 3, 0.4, 0.5 };
        add_hdf5_numeric_dataset(ghandle, "image_scale_factors", H5::PredType::NATIVE_DOUBLE, sc);
    }
    expect_validation_error(dir, "finite");
}

TEST(SpatialExperiment, MappingImageFormatsError) {
    auto dir = define_test_path("spatial_experiment");

    SpatialExperimentOptions opt(7, 5);
    opt.num_samples = 2;
    opt.num_images_per_sample = 3;

    {
        auto ghandle = mock_spatial_experiment(dir, opt);
        ghandle.unlink("image_formats");
        ghandle.createDataSet("image_formats", H5::PredType::NATIVE_INT, create_hdf5_dataspace(6));
    }
    expect_validation_error(dir, "UTF-8 encoded string");

    {
        auto ghandle = mock_spatial_experiment(dir, opt);
        ghandle.unlink("image_formats");
        ghandle.createDataSet("image_formats", H5::StrType(0, 2), H5S_SCALAR);
    }
    expect_validation_error(dir, "1-dimensional");

    {
        auto ghandle = mock_spatial_experiment(dir, opt);
        ghandle.unlink("image_formats");
        ghandle.createDataSet("image_formats", H5::StrType(0, 2), create_hdf5_dataspace(5));
    }
    expect_validation_error(dir, "same as that of");
}

/******************************************/

TEST(SpatialExperiment, FormatPngError) {
    auto dir = define_test_path("spatial_experiment");

    {
        SpatialExperimentOptions opt(7, 5);
        opt.num_samples = 1;
        opt.num_images_per_sample = 3;
        auto ghandle = mock_spatial_experiment(dir, opt);
        quick_text_write(dir / "images" / "1.png", "hooray");
    }
    expect_validation_error(dir, "PNG file");
}

static const std::vector<unsigned char>& get_tiff_signature() {
    static std::vector<unsigned char> tif_sig{ 77, 77, 0, 42 };
    return tif_sig;
}

TEST(SpatialExperiment, FormatTiffOkay) {
    auto dir = define_test_path("spatial_experiment");

    {
        SpatialExperimentOptions opt(6, 20);
        opt.num_samples = 4;
        opt.num_images_per_sample = 2;
        opt.add_images = false;

        const std::size_t num_images = opt.num_samples * opt.num_images_per_sample;
        std::vector<std::string> formats(num_images, "TIFF");
        auto ghandle = mock_spatial_experiment(dir, opt);
        add_hdf5_string_dataset(ghandle, "image_formats", formats);

        const auto& tif_sig = get_tiff_signature();
        auto idir = dir / "images";
        for (std::size_t i = 0; i < num_images; ++i) {
            std::string path = idir / (std::to_string(i) + ".tif");
            byteme::RawFileWriter writer(path.c_str(), {});
            writer.write(tif_sig.data(), tif_sig.size());
        }
    }

    test_validate(dir);
    EXPECT_EQ(test_height(dir), 6);
    EXPECT_EQ(test_dimensions(dir), (std::vector<std::size_t>{ 6, 20 }));
}

TEST(SpatialExperiment, FormatTiffError) {
    auto dir = define_test_path("spatial_experiment");

    {
        SpatialExperimentOptions opt(6, 20);
        opt.num_samples = 4;
        opt.num_images_per_sample = 2;
        opt.add_images = false;

        const std::size_t num_images = opt.num_samples * opt.num_images_per_sample;
        std::vector<std::string> formats(num_images, "TIFF");
        auto ghandle = mock_spatial_experiment(dir, opt);
        add_hdf5_string_dataset(ghandle, "image_formats", formats);

        const auto& tif_sig = get_tiff_signature();
        auto idir = dir / "images";
        for (std::size_t i = 0; i < num_images; ++i) {
            std::string path = idir / (std::to_string(i) + ".tif");
            byteme::RawFileWriter writer(path.c_str(), {});
            writer.write(tif_sig.data(), tif_sig.size());

        }
        quick_text_write(dir / "images" / "2.tif", "hooray"); // injecting an incorrect TIFF file.
    }
    expect_validation_error(dir, "TIFF file");
}

TEST(SpatialExperiment, FormatOtherOkay) {
    auto dir = define_test_path("spatial_experiment");

    {
        SpatialExperimentOptions opt(6, 20);
        opt.num_samples = 3;
        opt.num_images_per_sample = 4;
        opt.add_images = false;
        opt.version = "1.1";

        auto ghandle = mock_spatial_experiment(dir, opt);
        std::size_t num_images = opt.num_samples * opt.num_images_per_sample;
        std::vector<std::string> formats(num_images, "OTHER");
        add_hdf5_string_dataset(ghandle, "image_formats", formats);

        const auto& tif_sig = get_tiff_signature();
        auto idir = dir / "images";
        for (std::size_t i = 0; i < num_images; ++i) {
            auto odir = idir / std::to_string(i);
            initialize_directory(odir);
            quick_text_write(
                odir / "OBJECT",
                "{ \"type\": \"image_file\", \"image_file\": { \"version\": \"1.0\", \"format\": \"TIFF\" } }"
            );
            std::string path = odir / "file.tif";
            byteme::RawFileWriter writer(path.c_str(), {});
            writer.write(tif_sig.data(), tif_sig.size());
        }
    }

    test_validate(dir);
    EXPECT_EQ(test_height(dir), 6);
    EXPECT_EQ(test_dimensions(dir), (std::vector<std::size_t>{ 6, 20 }));
}

TEST(SpatialExperiment, FormatOtherError) {
    auto dir = define_test_path("spatial_experiment");

    SpatialExperimentOptions opt(6, 20);
    opt.num_samples = 3;
    opt.num_images_per_sample = 4;
    opt.add_images = false;
    std::size_t num_images = opt.num_samples * opt.num_images_per_sample;

    {
        auto ghandle = mock_spatial_experiment(dir, opt);
        std::vector<std::string> formats(12, "OTHER");
        add_hdf5_string_dataset(ghandle, "image_formats", formats);
    }
    expect_validation_error(dir, "OTHER");

    {
        auto optcopy = opt;
        optcopy.version = "1.1";
        auto ghandle = mock_spatial_experiment(dir, optcopy);
        std::vector<std::string> formats(12, "OTHER");
        add_hdf5_string_dataset(ghandle, "image_formats", formats);
        for (std::size_t i = 0; i < num_images; ++i) {
            mock_data_frame(dir / "images" / std::to_string(i), DataFrameOptions(5, {}));
        }
    }
    expect_validation_error(dir, "'IMAGE' interface");

    // Check that the image object is actually validated.
    {
        auto optcopy = opt;
        optcopy.version = "1.1";
        auto ghandle = mock_spatial_experiment(dir, optcopy);
        std::vector<std::string> formats(12, "OTHER");
        add_hdf5_string_dataset(ghandle, "image_formats", formats);

        std::size_t num_images = opt.num_samples * opt.num_images_per_sample;
        for (std::size_t i = 0; i < num_images; ++i) {
            auto odir = dir / "images" / std::to_string(i);
            initialize_directory(odir);
            quick_text_write(
                odir / "OBJECT",
                "{ \"type\": \"image_file\", \"image_file\": { \"version\": \"1.0\", \"format\": \"TIFF\" } }"
            );
            quick_text_write(odir / "file.tif", "howdy");
        }
    }
    expect_validation_error(dir, "incorrect signature");
}

TEST(SpatialExperiment, FormatMixedOkay) {
    auto dir = define_test_path("spatial_experiment");

    {
        SpatialExperimentOptions opt(7, 50);
        opt.num_samples = 5;
        opt.num_images_per_sample = 3;
        opt.add_images = false;
        opt.version = "1.1";

        auto ghandle = mock_spatial_experiment(dir, opt);
        std::size_t num_images = opt.num_samples * opt.num_images_per_sample;
        std::vector<std::string> formats(num_images);

        const auto& png_sig = get_png_signature();
        const auto& tif_sig = get_tiff_signature();
        auto idir = dir / "images";
        for (std::size_t i = 0; i < num_images; ++i) {
            switch (i % 3) {
                case 0:
                    {
                        std::string path = idir / (std::to_string(i) + ".png");
                        byteme::RawFileWriter writer(path.c_str(), {});
                        writer.write(png_sig.data(), png_sig.size());
                        formats[i] = "PNG";
                    }
                    break;
                case 1: 
                    {
                        std::string path = idir / (std::to_string(i) + ".tif");
                        byteme::RawFileWriter writer(path.c_str(), {});
                        writer.write(tif_sig.data(), tif_sig.size());
                        formats[i] = "TIFF";
                    }
                    break;
                case 2: 
                    {
                        auto odir = idir / std::to_string(i);
                        std::filesystem::create_directory(odir);
                        formats[i] = "OTHER";

                        std::string fmt;
                        if (i % 2 == 0) {
                            fmt = "TIFF";
                            std::string path = odir / "file.tif";
                            byteme::RawFileWriter writer(path.c_str(), {});
                            writer.write(tif_sig.data(), tif_sig.size());
                        } else {
                            fmt = "PNG";
                            std::string path = odir / "file.png";
                            byteme::RawFileWriter writer(path.c_str(), {});
                            writer.write(png_sig.data(), png_sig.size());
                        }
                        quick_text_write(
                            odir / "OBJECT",
                            "{ \"type\": \"image_file\", \"image_file\": { \"version\": \"1.0\", \"format\": \"" + fmt + "\" } }"
                        );
                    }
                    break;
            }
        }

        add_hdf5_string_dataset(ghandle, "image_formats", formats);
    }

    test_validate(dir);
    EXPECT_EQ(test_height(dir), 7);
    EXPECT_EQ(test_dimensions(dir), (std::vector<std::size_t>{ 7, 50 }));
}

TEST(SpatialExperiment, FormatUnknownError) {
    auto dir = define_test_path("spatial_experiment");

    {
        SpatialExperimentOptions opt(6, 20);
        opt.num_samples = 3;
        opt.num_images_per_sample = 4;
        opt.add_images = false;

        auto ghandle = mock_spatial_experiment(dir, opt);
        std::vector<std::string> formats(12, "FOOBAR");
        add_hdf5_string_dataset(ghandle, "image_formats", formats);
    }
    expect_validation_error(dir, "FOOBAR");
}

TEST(SpatialExperiment, ImagesExcessError) {
    auto dir = define_test_path("spatial_experiment");

    {
        SpatialExperimentOptions opt(7, 5);
        opt.num_samples = 3;
        opt.num_images_per_sample = 1;
        mock_spatial_experiment(dir, opt);
        quick_text_write(dir / "images" / "3", "hi!");
    }
    expect_validation_error(dir, "more objects than expected");
}

/******************************************/

TEST(SpatialExperiment, NoFormatOkay) {
    auto dir = define_test_path("spatial_experiment");

    {
        SpatialExperimentOptions opt(7, 50);
        opt.num_samples = 4;
        opt.num_images_per_sample = 2;
        opt.add_images = false;
        opt.version = "1.3";
        mock_spatial_experiment(dir, opt);

        auto idir = dir / "images";
        std::size_t num_images = opt.num_samples * opt.num_images_per_sample;
        const auto& png_sig = get_png_signature();
        for (std::size_t i = 0; i < num_images; ++i) {
            auto odir = idir / std::to_string(i);
            std::filesystem::create_directory(odir);
            quick_text_write(
                odir / "OBJECT",
                "{ \"type\": \"image_file\", \"image_file\": { \"version\": \"1.0\", \"format\": \"PNG\" } }"
            );
            std::string path = odir / "file.png";
            byteme::RawFileWriter writer(path.c_str(), {});
            writer.write(png_sig.data(), png_sig.size());
        }
    }

    test_validate(dir);
    EXPECT_EQ(test_height(dir), 7);
    EXPECT_EQ(test_dimensions(dir), (std::vector<std::size_t>{ 7, 50 }));
}

TEST(SpatialExperiment, NoFormatError) {
    auto dir = define_test_path("spatial_experiment");

    {
        SpatialExperimentOptions opt(7, 5);
        opt.num_samples = 3;
        opt.num_images_per_sample = 1;
        opt.add_images = false;
        opt.version = "1.2";
        mock_spatial_experiment(dir, opt);
    }

    H5Eset_auto(H5E_DEFAULT, NULL, NULL);
    expect_validation_error(dir, "failed to validate 'image_formats'");
}
