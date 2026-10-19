#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "mock_multi_sample_dataset.h"
#include "mock_summarized_experiment.h"
#include "utils.h"

#include <string>
#include <vector>
#include <filesystem>
#include <stdexcept>

TEST(MultiSampleDataset, Okay) {
    auto dir = define_test_path("multi_sample_dataset");

    // One experiment.
    {
        MultiSampleDatasetOptions opt(3, { SummarizedExperimentOptions(18, 7) });
        mock_multi_sample_dataset(dir, opt);
    }
    test_validate(dir);

    // Two experiments.
    {
        MultiSampleDatasetOptions opt(3, { SummarizedExperimentOptions(18, 7), SummarizedExperimentOptions(21, 11) });
        mock_multi_sample_dataset(dir, opt);
    }
    test_validate(dir);

    // Three experiments.
    {
        MultiSampleDatasetOptions opt(3, { SummarizedExperimentOptions(18, 7), SummarizedExperimentOptions(21, 11), SummarizedExperimentOptions(5, 2) });
        mock_multi_sample_dataset(dir, opt);
    }
    test_validate(dir);
}

TEST(MultiSampleDataset, EmptyOkay) {
    auto dir = define_test_path("multi_sample_dataset");

    // No experiments.
    {
        MultiSampleDatasetOptions opt(3, {});
        mock_multi_sample_dataset(dir, opt);
    }
    test_validate(dir);

    // No samples.
    {
        MultiSampleDatasetOptions opt(0, { SummarizedExperimentOptions(10, 0), SummarizedExperimentOptions(5, 0) });
        mock_multi_sample_dataset(dir, opt);
    }
    test_validate(dir);

    // No experiment directory at all.
    {
        MultiSampleDatasetOptions opt(3, {});
        mock_multi_sample_dataset(dir, opt);
        std::filesystem::remove_all(dir / "experiments");
    }
    test_validate(dir);

    // No mapping file at all.
    {
        MultiSampleDatasetOptions opt(3, {});
        mock_multi_sample_dataset(dir, opt);
        std::filesystem::remove_all(dir / "sample_map.h5");
    }
    test_validate(dir);
}

TEST(MultiSampleDataset, VersionError) {
    auto dir = define_test_path("multi_sample_dataset");

    {
        MultiSampleDatasetOptions opt(3, {});
        opt.version = "2.0";
        mock_multi_sample_dataset(dir, opt);
    }
    expect_validation_error(dir, "unsupported version");
}

TEST(MultiSampleDataset, SampleDataError) {
    auto dir = define_test_path("multi_sample_dataset");

    {
        MultiSampleDatasetOptions opt(3, {});
        opt.has_sample_data = false;
        mock_multi_sample_dataset(dir, opt);
        mock_simple_list(dir / "sample_data");
    }
    expect_validation_error(dir, "DATA_FRAME");

    // Check that data frame validation is performed.
    {
        MultiSampleDatasetOptions opt(3, {});
        mock_multi_sample_dataset(dir, opt);
        H5::H5File handle(dir / "sample_data" / "basic_columns.h5", H5F_ACC_RDWR);
        auto ghandle = handle.openGroup("data_frame");
        ghandle.removeAttr("row-count");
        add_hdf5_string_attribute(ghandle, "row-count", "foobar");
    }
    expect_validation_error(dir, "64-bit unsigned integer");
}

TEST(MultiSampleDataset, ExperimentsError) {
    auto dir = define_test_path("multi_sample_dataset");

    // Check that names are actually validated.
    {
        MultiSampleDatasetOptions opt(3, { SummarizedExperimentOptions(18, 7), SummarizedExperimentOptions(5, 4) });
        mock_multi_sample_dataset(dir, opt);
        quick_text_write(dir / "experiments" / "names.json", "[ true ]");
    }
    expect_validation_error(dir, "array of strings");

    {
        MultiSampleDatasetOptions opt(3, { SummarizedExperimentOptions(18, 7), SummarizedExperimentOptions(5, 4) });
        mock_multi_sample_dataset(dir, opt);
        quick_text_write(dir / "experiments" / "names.json", "[ \"a\", \"b\", \"c\" ]");
        mock_data_frame(dir / "experiments" / "2", DataFrameOptions(3, {}));
    }
    expect_validation_error(dir, "SUMMARIZED_EXPERIMENT");

    // Test that we actually validate the SEs.
    {
        MultiSampleDatasetOptions opt(3, { SummarizedExperimentOptions(18, 7), SummarizedExperimentOptions(5, 4) });
        mock_multi_sample_dataset(dir, opt);
        quick_text_write(dir / "experiments" / "0" / "assays" / "names.json", "[ \"a\", \"a\" ]");
    }
    expect_validation_error(dir, "duplicated name");

    {
        MultiSampleDatasetOptions opt(3, { SummarizedExperimentOptions(18, 7), SummarizedExperimentOptions(5, 4) });
        mock_multi_sample_dataset(dir, opt);
        initialize_directory(dir / "experiments" / "foo");
    }
    expect_validation_error(dir, "more objects than expected");
}

TEST(MultiSampleDataset, SampleMapError) {
    auto dir = define_test_path("multi_sample_dataset");

    {
        MultiSampleDatasetOptions opt(10, { SummarizedExperimentOptions(20, 30) });
        auto ghandle = mock_multi_sample_dataset(dir, opt);
        ghandle.unlink("0");
        ghandle.createDataSet("0", H5::PredType::NATIVE_INT, create_hdf5_dataspace(30));
    }
    expect_validation_error(dir, "64-bit unsigned integer");

    {
        MultiSampleDatasetOptions opt(10, { SummarizedExperimentOptions(20, 30), SummarizedExperimentOptions(5, 40) });
        auto ghandle = mock_multi_sample_dataset(dir, opt);
        ghandle.unlink("1");
        ghandle.createDataSet("1", H5::PredType::NATIVE_UINT8, H5S_SCALAR);
    }
    expect_validation_error(dir, "1-dimensional dataset");

    {
        MultiSampleDatasetOptions opt(10, { SummarizedExperimentOptions(15, 5), SummarizedExperimentOptions(5, 2), SummarizedExperimentOptions(12, 10) });
        auto ghandle = mock_multi_sample_dataset(dir, opt);
        ghandle.unlink("2");
        ghandle.createDataSet("2", H5::PredType::NATIVE_UINT8, create_hdf5_dataspace(11));
    }
    expect_validation_error(dir, "should equal the number of columns");

    {
        MultiSampleDatasetOptions opt(3, { SummarizedExperimentOptions(15, 5), SummarizedExperimentOptions(5, 2) });
        auto ghandle = mock_multi_sample_dataset(dir, opt);
        ghandle.unlink("0");
        std::vector<int> new_mapping(5, 2);
        new_mapping.back() = 3;
        add_hdf5_numeric_dataset(ghandle, "0", H5::PredType::NATIVE_UINT8, new_mapping);
    }
    expect_validation_error(dir, "less than the number of samples");

    {
        MultiSampleDatasetOptions opt(3, { SummarizedExperimentOptions(15, 5), SummarizedExperimentOptions(5, 2) });
        auto ghandle = mock_multi_sample_dataset(dir, opt);
        ghandle.createGroup("2");
    }
    expect_validation_error(dir, "more objects present");

    // Validation is still performed if there are no experiments, as long as the mapping file exists.
    {
        MultiSampleDatasetOptions opt(10, {});
        auto ghandle = mock_multi_sample_dataset(dir, opt);
        ghandle.createGroup("0");
    }
    expect_validation_error(dir, "more objects present");
}

TEST(MultiSampleDataset, MetadataOkay) {
    auto dir = define_test_path("multi_sample_dataset");

    {
        MultiSampleDatasetOptions opt(3, { SummarizedExperimentOptions(18, 7) });
        mock_multi_sample_dataset(dir, opt);
        mock_simple_list(dir / "other_data");
    }
    test_validate(dir);
}

TEST(MultiSampleDataset, MetadataError) {
    auto dir = define_test_path("multi_sample_dataset");

    // Check that we actually validate the other_data.
    {
        MultiSampleDatasetOptions opt(3, { SummarizedExperimentOptions(18, 7) });
        mock_multi_sample_dataset(dir, opt);
        mock_data_frame(dir / "other_data", DataFrameOptions(7, {}));
    }
    expect_validation_error(dir, "'SIMPLE_LIST' interface");
}
