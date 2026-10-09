#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "mock_summarized_experiment.h"
#include "mock_dense_array.h"
#include "utils.h"

#include <string>
#include <vector>
#include <filesystem>
#include <stdexcept>

TEST(SummarizedExperiment, AssaysOkay) {
    auto dir = define_test_path("summarized_experiment");

    {
        mock_summarized_experiment(dir, SummarizedExperimentOptions(10, 20));
    }
    {
        test_validate(dir);
        EXPECT_EQ(test_height(dir), 10);
        EXPECT_EQ(test_dimensions(dir), (std::vector<std::size_t>{10, 20}));
    }

    // Multiple assays.
    {
        mock_summarized_experiment(dir, SummarizedExperimentOptions(30, 15, 3));
    }
    {
        test_validate(dir);
        EXPECT_EQ(test_height(dir), 30);
        EXPECT_EQ(test_dimensions(dir), (std::vector<std::size_t>{30, 15}));
    }

    // No assays.
    {
        mock_summarized_experiment(dir, SummarizedExperimentOptions(50, 1, 0));
    }
    {
        test_validate(dir);
        EXPECT_EQ(test_height(dir), 50);
        EXPECT_EQ(test_dimensions(dir), (std::vector<std::size_t>{50, 1}));
    }

    // High-dimensional assay.
    {
        mock_summarized_experiment(dir, SummarizedExperimentOptions(20, 10, 2));
        std::filesystem::remove_all(dir / "assays" / "1");
        mock_dense_array(dir / "assays" / "1", DenseArrayType::NUMBER, { 20, 10, 5 });
    }
    {
        test_validate(dir);
        EXPECT_EQ(test_height(dir), 20);
        EXPECT_EQ(test_dimensions(dir), (std::vector<std::size_t>{20, 10}));
    }
}

TEST(SummarizedExperiment, MissingDirectoryOkay) {
    auto dir = define_test_path("summarized_experiment");

    // No row/column data.
    {
        SummarizedExperimentOptions opts(5, 99, 2);
        opts.has_row_data = false;
        opts.has_column_data = false;
        mock_summarized_experiment(dir, opts);
    }
    {
        test_validate(dir);
        EXPECT_EQ(test_height(dir), 5);
        EXPECT_EQ(test_dimensions(dir), (std::vector<std::size_t>{5, 99}));
    }

    // No other metadata.
    {
        SummarizedExperimentOptions opts(99, 5, 1);
        opts.has_other_data = false;
        mock_summarized_experiment(dir, opts);
    }
    {
        test_validate(dir);
        EXPECT_EQ(test_height(dir), 99);
        EXPECT_EQ(test_dimensions(dir), (std::vector<std::size_t>{99, 5}));
    }

    // No assay directory at all.
    {
        mock_summarized_experiment(dir, SummarizedExperimentOptions(20, 10, 0));
        std::filesystem::remove_all(dir / "assays");
    }
    {
        test_validate(dir);
        EXPECT_EQ(test_height(dir), 20);
        EXPECT_EQ(test_dimensions(dir), (std::vector<std::size_t>{20, 10}));
    }
}

TEST(SummarizedExperiment, EmptyOkay) {
    auto dir = define_test_path("summarized_experiment");

    // No rows.
    {
        mock_summarized_experiment(dir, SummarizedExperimentOptions(0, 20, 3));
    }
    {
        test_validate(dir);
        EXPECT_EQ(test_height(dir), 0);
        EXPECT_EQ(test_dimensions(dir), (std::vector<std::size_t>{0, 20}));
    }

    // No columns.
    {
        mock_summarized_experiment(dir, SummarizedExperimentOptions(20, 0, 3));
    }
    {
        test_validate(dir);
        EXPECT_EQ(test_height(dir), 20);
        EXPECT_EQ(test_dimensions(dir), (std::vector<std::size_t>{20, 0}));
    }
}

/*****************************************/

TEST(SummarizedExperiment, VersionError) {
    auto dir = define_test_path("summarized_experiment");

    {
        initialize_directory_simple(dir, "summarized_experiment", "2.0");
    }
    expect_validation_error(dir, "unsupported version");
}

TEST(SummarizedExperiment, DimensionsError) {
    auto dir = define_test_path("summarized_experiment");

    {
        quick_text_write(
            dir / "OBJECT",
            "{ \"type\": \"summarized_experiment\", \"summarized_experiment\": { \"version\": \"1.0\", \"dimensions\": null } }"
        );
    }
    expect_validation_error(dir, "failed to read 'dimensions'");
}

/*****************************************/

TEST(SummarizedExperiment, AssayNamesError) {
    auto dir = define_test_path("summarized_experiment");

    {
        mock_summarized_experiment(dir, SummarizedExperimentOptions(5, 6));
        quick_text_write(
            dir / "assays" / "names.json",
            "[ 1, 2, 3 ]"
        );
    }
    expect_validation_error(dir, "expected an array of strings");

    {
        mock_summarized_experiment(dir, SummarizedExperimentOptions(5, 6, 2));
        mock_dense_array(dir / "assays" / "2", DenseArrayType::NUMBER, { 5, 6 });
    }
    expect_validation_error(dir, "more objects than expected");
}

TEST(SummarizedExperiment, AssayContentError) {
    auto dir = define_test_path("summarized_experiment");

    // Check that validate() is actually called on the assays.
    {
        mock_summarized_experiment(dir, SummarizedExperimentOptions(5, 6, 1));
        H5::H5File handle(dir / "assays" / "0" / "array.h5", H5F_ACC_RDWR);
        auto ghandle = handle.openGroup("dense_array");
        ghandle.removeAttr("type");
        add_hdf5_string_attribute(ghandle, "type", "foobar");
    }
    expect_validation_error(dir, "foobar");
}

TEST(SummarizedExperiment, AssayDimensionsError) {
    auto dir = define_test_path("summarized_experiment");

    {
        mock_summarized_experiment(dir, SummarizedExperimentOptions(5, 6, 3));
        std::filesystem::remove_all(dir / "assays" / "1");
        mock_dense_array(dir / "assays" / "1", DenseArrayType::STRING, { 1 });
    }
    expect_validation_error(dir, "two or more dimensions");

    {
        mock_summarized_experiment(dir, SummarizedExperimentOptions(5, 6, 3));
        std::filesystem::remove_all(dir / "assays" / "2");
        mock_dense_array(dir / "assays" / "2", DenseArrayType::STRING, { 7, 6 });
    }
    expect_validation_error(dir, "number of rows");

    {
        mock_summarized_experiment(dir, SummarizedExperimentOptions(5, 6, 3));
        std::filesystem::remove_all(dir / "assays" / "0");
        mock_dense_array(dir / "assays" / "0", DenseArrayType::STRING, { 5, 5 });
    }
    expect_validation_error(dir, "number of columns");
}

/*****************************************/

TEST(SummarizedExperimentTest, RowDataError) {
    auto dir = define_test_path("summarized_experiment");

    {
        SummarizedExperimentOptions opt(60, 3, 2);
        opt.has_row_data = false;
        mock_summarized_experiment(dir, opt);
        mock_simple_list(dir / "row_data");
    }
    expect_validation_error(dir, "DATA_FRAME");

    // Check that the row data is actually validated.
    {
        SummarizedExperimentOptions opt(60, 3, 2);
        opt.has_row_data = false;
        mock_summarized_experiment(dir, opt);
        std::vector<DataFrameColumnDetails> cols(1);
        mock_data_frame(dir / "row_data", 60, cols);
    }
    expect_validation_error(dir, "empty strings");

    {
        SummarizedExperimentOptions opt(60, 3, 2);
        opt.has_row_data = false;
        mock_summarized_experiment(dir, opt);
        std::vector<DataFrameColumnDetails> cols(1);
        cols[0].name = "akari";
        mock_data_frame(dir / "row_data", 61, cols);
    }
    expect_validation_error(dir, "number of rows");
}

TEST(SummarizedExperimentTest, ColumnDataError) {
    auto dir = define_test_path("summarized_experiment");

    {
        SummarizedExperimentOptions opt(30, 20, 0);
        opt.has_column_data = false;
        mock_summarized_experiment(dir, opt);
        mock_simple_list(dir / "column_data");
    }
    expect_validation_error(dir, "DATA_FRAME");

    // Check that the column data is actually validated.
    {
        SummarizedExperimentOptions opt(30, 20, 0);
        opt.has_column_data = false;
        mock_summarized_experiment(dir, opt);
        std::vector<DataFrameColumnDetails> cols(1);
        mock_data_frame(dir / "column_data", 20, cols);
    }
    expect_validation_error(dir, "empty strings");

    {
        SummarizedExperimentOptions opt(30, 20, 0);
        opt.has_column_data = false;
        mock_summarized_experiment(dir, opt);
        std::vector<DataFrameColumnDetails> cols(1);
        cols[0].name = "ai";
        mock_data_frame(dir / "column_data", 19, cols);
    }
    expect_validation_error(dir, "number of columns");
}

TEST(SummarizedExperimentTest, MetadataError) {
    auto dir = define_test_path("summarized_experiment");

    // Check that metadata is actually validated.
    {
        SummarizedExperimentOptions opt(30, 20, 0);
        opt.has_other_data = false;
        mock_summarized_experiment(dir, opt);
        initialize_directory_simple(dir / "other_data", "foobar", "1.2");
    }
    expect_validation_error(dir, "SIMPLE_LIST");
}
