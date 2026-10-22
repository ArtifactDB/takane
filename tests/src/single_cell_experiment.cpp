#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "mock_single_cell_experiment.h"
#include "utils.h"

#include <string>
#include <vector>
#include <filesystem>
#include <stdexcept>

TEST(SingleCellExperiment, Okay) {
    auto dir = define_test_path("single_cell_experiment");

    {
        mock_single_cell_experiment(dir, SingleCellExperimentOptions(23, 37));
    }
    {
        test_validate(dir);
        EXPECT_EQ(test_height(dir), 23);
        EXPECT_EQ(test_dimensions(dir), (std::vector<std::size_t>{ 23, 37 }));
    }

    // Throwing in a main experiment name.
    {
        SingleCellExperimentOptions opt(11, 55);
        opt.main_exp_name = "foobar";
        mock_single_cell_experiment(dir, opt);
    }
    {
        test_validate(dir);
        EXPECT_EQ(test_height(dir), 11);
        EXPECT_EQ(test_dimensions(dir), (std::vector<std::size_t>{ 11, 55 }));
    }

    // Multiple reduced dimensions and alternative experiments.
    {
        SingleCellExperimentOptions opt(8, 7);
        opt.num_reduced_dims = 3;
        opt.num_alt_exps = 2;
        mock_single_cell_experiment(dir, opt);
    }
    {
        test_validate(dir);
        EXPECT_EQ(test_height(dir), 8);
        EXPECT_EQ(test_dimensions(dir), (std::vector<std::size_t>{ 8, 7 }));
    }

    // Works with a SE subclass as an alternative experiment.
    {
        mock_single_cell_experiment(dir, SingleCellExperimentOptions(31, 14));
        std::filesystem::remove_all(dir / "alternative_experiments" / "0");
        mock_ranged_summarized_experiment(dir / "alternative_experiments" / "0", RangedSummarizedExperimentOptions(11, 14));
    }
    {
        test_validate(dir);
        EXPECT_EQ(test_height(dir), 31);
        EXPECT_EQ(test_dimensions(dir), (std::vector<std::size_t>{ 31, 14 }));
    }
}

TEST(SingleCellExperiment, EmptyOkay) {
    auto dir = define_test_path("single_cell_experiment");

    // Zero reduced dimensions or alternative experiments.
    {
        SingleCellExperimentOptions opt(31, 14);
        opt.num_reduced_dims = 0;
        opt.num_alt_exps = 0;
        mock_single_cell_experiment(dir, opt);
    }
    {
        test_validate(dir);
        EXPECT_EQ(test_height(dir), 31);
        EXPECT_EQ(test_dimensions(dir), (std::vector<std::size_t>{ 31, 14 }));
    }

    // No reduced dimensions or alternative experiments directory.
    {
        mock_single_cell_experiment(dir, SingleCellExperimentOptions(15, 32));
        std::filesystem::remove_all(dir / "reduced_dimensions");
        std::filesystem::remove_all(dir / "alternative_experiments");
    }
    {
        test_validate(dir);
        EXPECT_EQ(test_height(dir), 15);
        EXPECT_EQ(test_dimensions(dir), (std::vector<std::size_t>{ 15, 32 }));
    }
}

/********************************************/

TEST(SingleCellExperiment, BaseError) {
    auto dir = define_test_path("single_cell_experiment");

    // Check that the base RSE is actually validated.
    {
        SingleCellExperimentOptions opt(15, 30);
        opt.base.row_ranges = RangedSummarizedExperimentRowRangesType::NONE;
        mock_single_cell_experiment(dir, opt);
        mock_data_frame(dir / "row_ranges", DataFrameOptions(15, {}));
    }
    expect_validation_error(dir, "'genomic_ranges', 'genomic_ranges_list'");
}

TEST(SingleCellExperiment, VersionError) {
    auto dir = define_test_path("single_cell_experiment");

    {
        SingleCellExperimentOptions opt(12, 40);
        opt.version = "2.0";
        mock_single_cell_experiment(dir, opt);
    }
    expect_validation_error(dir, "unsupported version");
}

TEST(SingleCellExperiment, MainExpNameError) {
    auto dir = define_test_path("single_cell_experiment");

    {
        mock_single_cell_experiment(dir, SingleCellExperimentOptions(12, 40));

        std::string objpath = dir / "OBJECT";
        auto contents = millijson::parse_file(objpath.c_str(), {});
        auto optr = reinterpret_cast<millijson::Object*>(contents.get());
        auto sptr = reinterpret_cast<millijson::Object*>(optr->value()["single_cell_experiment"].get());
        sptr->value()["main_experiment_name"] = std::shared_ptr<millijson::Base>(new millijson::Nothing);
        dump_json(contents.get(), dir / "OBJECT");
    }
    expect_validation_error(dir, "to be a string");

    {
        mock_single_cell_experiment(dir, SingleCellExperimentOptions(12, 40));

        std::string objpath = dir / "OBJECT";
        auto contents = millijson::parse_file(objpath.c_str(), {});
        auto optr = reinterpret_cast<millijson::Object*>(contents.get());
        auto sptr = reinterpret_cast<millijson::Object*>(optr->value()["single_cell_experiment"].get());
        sptr->value()["main_experiment_name"] = std::shared_ptr<millijson::Base>(new millijson::String(""));
        dump_json(contents.get(), dir / "OBJECT");
    }
    expect_validation_error(dir, "non-empty string");
}

/********************************************/

TEST(SingleCellExperiment, ReducedDimensionsError) {
    auto dir = define_test_path("single_cell_experiment");

    {
        mock_single_cell_experiment(dir, SingleCellExperimentOptions(13, 39));
        quick_text_write(dir / "reduced_dimensions" / "names.json", "[ true ]");
    }
    expect_validation_error(dir, "array of strings");

    // Check that validation is actually performed on the inner entries.
    {
        mock_single_cell_experiment(dir, SingleCellExperimentOptions(13, 39));
        H5::H5File handle(dir / "reduced_dimensions" / "0" / "array.h5", H5F_ACC_RDWR);
        auto ghandle = handle.openGroup("dense_array");
        ghandle.removeAttr("type");
        add_hdf5_string_attribute(ghandle, "type", "foobar");
    }
    expect_validation_error(dir, "foobar");

    // Check what happens if someone reports a zero-dimension array.
    {
        mock_single_cell_experiment(dir, SingleCellExperimentOptions(13, 39));
        std::filesystem::remove_all(dir / "reduced_dimensions" / "0");
        initialize_directory_simple(dir / "reduced_dimensions" / "0", "foobar", "2.0");
    }
    {
        takane::Options opt;
        opt.custom_validate["foobar"] = [&](const std::filesystem::path&, const takane::ObjectMetadata&, const takane::Options&) -> void {};
        opt.custom_dimensions["foobar"] = [&](const std::filesystem::path&, const takane::ObjectMetadata&, const takane::Options&) -> std::vector<std::size_t> { return {}; };
        expect_validation_error(dir, "at least one dimension", opt);
    }

    {
        mock_single_cell_experiment(dir, SingleCellExperimentOptions(13, 39));
        std::filesystem::remove_all(dir / "reduced_dimensions" / "0");
        mock_dense_array(dir / "reduced_dimensions" / "0", DenseArrayOptions({ 144, 10 }, DenseArrayType::STRING));
    }
    expect_validation_error(dir, "same number of rows");

    {
        mock_single_cell_experiment(dir, SingleCellExperimentOptions(13, 39));
        mock_dense_array(dir / "reduced_dimensions" / "1", DenseArrayOptions({ 39, 5 }, DenseArrayType::STRING));
    }
    expect_validation_error(dir, "more objects than expected");
}

/********************************************/

TEST(SingleCellExperiment, AlternativeExperimentsError) {
    auto dir = define_test_path("single_cell_experiment");

    {
        mock_single_cell_experiment(dir, SingleCellExperimentOptions(13, 39));
        quick_text_write(dir / "alternative_experiments" / "names.json", "[ true ]");
    }
    expect_validation_error(dir, "array of strings");

    {
        SingleCellExperimentOptions opt(13, 39);
        opt.num_alt_exps = 2;
        opt.main_exp_name = "altexps-1";
        mock_single_cell_experiment(dir, opt);
    }
    expect_validation_error(dir, "main experiment name");

    {
        mock_single_cell_experiment(dir, SingleCellExperimentOptions(13, 39));
        std::filesystem::remove_all(dir / "alternative_experiments" / "0");
        mock_dense_array(dir / "alternative_experiments" / "0", DenseArrayOptions({ 13, 39 }, DenseArrayType::STRING));
    }
    expect_validation_error(dir, "SUMMARIZED_EXPERIMENT");

    // Check that validation is actually performed on the inner entries.
    {
        mock_single_cell_experiment(dir, SingleCellExperimentOptions(13, 39));
        quick_text_write(dir / "alternative_experiments" / "0" / "assays" / "names.json", "{ \"foo\": null }");
    }
    expect_validation_error(dir, "array");

    {
        SingleCellExperimentOptions opt(13, 39);
        opt.num_alt_exps = 2;
        mock_single_cell_experiment(dir, opt);
        std::filesystem::remove_all(dir / "alternative_experiments" / "1");
        mock_summarized_experiment(dir / "alternative_experiments" / "1", SummarizedExperimentOptions(22, 40));
    }
    expect_validation_error(dir, "number of columns");

    {
        mock_single_cell_experiment(dir, SingleCellExperimentOptions(13, 39));
        mock_summarized_experiment(dir / "alternative_experiments" / "1", SummarizedExperimentOptions(13, 39));
    }
    expect_validation_error(dir, "more objects than expected");
}
