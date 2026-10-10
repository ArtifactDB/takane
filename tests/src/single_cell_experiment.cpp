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

/********************************************/

TEST(SingleCellExperiment, BaseError) {
    auto dir = define_test_path("single_cell_experiment");

    // Check that the base RSE is actually validated.
    {
        SingleCellExperimentOptions opt(15, 30);
        opt.use_grl = false;
        mock_single_cell_experiment(dir, opt);
        mock_data_frame(dir / "row_ranges", 15, {});
    }
    expect_validation_error(dir, "'genomic_ranges', 'genomic_ranges_list'");
}

TEST(SingleCellExperiment, VersionError) {
    auto dir = define_test_path("single_cell_experiment");

    {
        mock_single_cell_experiment(dir, SingleCellExperimentOptions(12, 40));

        std::string objpath = dir / "OBJECT";
        auto contents = millijson::parse_file(objpath.c_str(), {});
        auto optr = reinterpret_cast<millijson::Object*>(contents.get());
        add_single_cell_experiment_metadata(optr, "2.0", "foobar");
        dump_json(contents.get(), dir / "OBJECT");
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
        mock_dense_array(dir / "reduced_dimensions" / "0", DenseArrayType::STRING, { 144, 10 });
    }
    expect_validation_error(dir, "same number of rows");

    {
        mock_single_cell_experiment(dir, SingleCellExperimentOptions(13, 39));
        mock_dense_array(dir / "reduced_dimensions" / "1", DenseArrayType::STRING, { 144, 10 });
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
        mock_dense_array(dir / "alternative_experiments" / "0", DenseArrayType::STRING, { 13, 39 });
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

//    single_cell_experiment::Options options(20, 15);
//    single_cell_experiment::mock(dir, options);
//
//    // Hits the base SE checks.
//
//    // Hits the base RSE checks.
//    auto opath = dir / "OBJECT";
//    auto parsed = millijson::parse_file(opath.c_str(), {});
//    {
//        ::summarized_experiment::add_object_metadata(parsed.get(), "1.0", 99, 23);
//        json_utils::dump(parsed.get(), opath);
//    }
//    expect_error("'ranged_summarized_experiment'");
//
//    // Check that SCE metadata is recognized.
//    {
//        ::ranged_summarized_experiment::add_object_metadata(parsed.get(), "1.0");
//        ::single_cell_experiment::add_object_metadata(parsed.get(), "2.0", "");
//        json_utils::dump(parsed.get(), opath);
//    }
//    expect_error("unsupported version");
//
//    single_cell_experiment::Options options(20, 15);
//    single_cell_experiment::mock(dir, options);
//    test_validate(dir); 
//    EXPECT_EQ(test_height(dir), 20);
//    std::vector<size_t> expected_dim{ 20, 15 };
//    EXPECT_EQ(test_dimensions(dir), expected_dim);
//}

//TEST_F(SingleCellExperimentTest, ReducedDims) {
//    single_cell_experiment::Options options(20, 15);
//    options.num_reduced_dims = 2;
//    options.num_alt_exps = 0;
//    single_cell_experiment::mock(dir, options);
//
//    {
//        simple_list::mock(dir / "reduced_dimensions" / "0");
//    }
//    expect_error("no registered 'dimensions' function");
//
//    {
//        dense_array::mock(dir / "reduced_dimensions" / "0", dense_array::Type::INTEGER, {});
//    }
//    expect_error("at least one dimension");
//
//    {
//        dense_array::mock(dir / "reduced_dimensions" / "0", dense_array::Type::INTEGER, { 20, 10 });
//    }
//    expect_error("number of rows");
//
//    {
//        dense_array::mock(dir / "reduced_dimensions" / "0", dense_array::Type::INTEGER, { 15, 5 });
//        dense_array::mock(dir / "reduced_dimensions" / "foobar", dense_array::Type::INTEGER, { 20, 10 });
//    }
//    expect_error("more objects than expected");
//
//    // Absence of reduced_dimensions is allowed.
//    {
//        std::filesystem::remove_all(dir / "reduced_dimensions");
//    }
//    test_validate(dir); 
//}
//
//TEST_F(SingleCellExperimentTest, AlternativeExps) {
//    single_cell_experiment::Options options(100, 20);
//    options.num_reduced_dims = 0;
//    options.num_alt_exps = 2;
//    single_cell_experiment::mock(dir, options);
//
//    {
//        dense_array::mock(dir / "alternative_experiments" / "0", dense_array::Type::INTEGER, { 100, 20 });
//    }
//    expect_error("'SUMMARIZED_EXPERIMENT' interface");
//
//    {
//        summarized_experiment::mock(dir / "alternative_experiments" / "0", summarized_experiment::Options(10, 100));
//    }
//    expect_error("same number of columns");
//
//    {
//        summarized_experiment::mock(dir / "alternative_experiments" / "0", summarized_experiment::Options(10, 20));
//        summarized_experiment::mock(dir / "alternative_experiments" / "foobar", summarized_experiment::Options(10, 5));
//    }
//    expect_error("more objects than expected");
//
//    // Absence of alternative_experiments is allowed.
//    {
//        std::filesystem::remove_all(dir / "alternative_experiments");
//    }
//    test_validate(dir); 
//}
//
//TEST_F(SingleCellExperimentTest, MainExperimentName) {
//    single_cell_experiment::Options options(100, 20);
//    options.num_reduced_dims = 2;
//    options.num_alt_exps = 2;
//    single_cell_experiment::mock(dir, options);
//
//    auto opath = dir / "OBJECT";
//    auto parsed = millijson::parse_file(opath.c_str(), {});
//    auto& toplevel = reinterpret_cast<millijson::Object*>(parsed.get())->value();
//    auto& scemap = reinterpret_cast<millijson::Object*>(toplevel["single_cell_experiment"].get())->value();
//    {
//        scemap["main_experiment_name"] = std::shared_ptr<millijson::Base>(new millijson::Number(2));
//        json_utils::dump(parsed.get(), opath);
//    }
//    expect_error("to be a string");
//
//    {
//        scemap["main_experiment_name"] = std::shared_ptr<millijson::Base>(new millijson::String(""));
//        json_utils::dump(parsed.get(), opath);
//    }
//    expect_error("empty string");
//
//    {
//        scemap["main_experiment_name"] = std::shared_ptr<millijson::Base>(new millijson::String("foo"));
//        json_utils::dump(parsed.get(), opath);
//        std::ofstream ahandle(dir / "alternative_experiments" / "names.json");
//        ahandle << "[ \"foo\", \"bar\" ]";
//    }
//    expect_error("not overlap");
//
//    // Finally success.
//    options.main_exp_name = "stuff";
//    single_cell_experiment::mock(dir, options);
//    test_validate(dir);
//}
