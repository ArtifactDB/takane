#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "takane/utils_other.hpp"

#include "mock_data_frame.h"
#include "mock_simple_list.h"
#include "utils.h"

class IterateStreamTest : public ::testing::TestWithParam<int> {};

TEST_P(IterateStreamTest, IterateStream) {
    auto dir = define_test_path("utils_other");
    initialize_directory(dir);

    std::vector<std::int32_t> values(100);
    std::iota(values.begin(), values.end(), -50);

    {
        H5::H5File handle(dir / "foo.h5", H5F_ACC_TRUNC);
        add_hdf5_numeric_dataset(handle, "bar", H5::PredType::NATIVE_INT32, values);
    }

    H5::H5File handle(dir / "foo.h5", H5F_ACC_RDONLY);
    auto dhandle = handle.openDataSet("bar");

    ritsuko::hdf5::Stream1dNumericDatasetOptions opt;
    opt.contiguous_chunk_size = GetParam();
    ritsuko::hdf5::Stream1dNumericDataset<std::int32_t> stream(&dhandle, values.size(), opt);

    std::vector<std::int32_t> output(values.size());
    takane::iterate_stream<std::int32_t>(
        stream,
        [&](hsize_t i, std::int32_t payload) -> void {
            output[i] = payload;
        }
    );

    EXPECT_EQ(output, values);
}

TEST_P(IterateStreamTest, NumericStreamIterator) {
    auto dir = define_test_path("utils_other");
    initialize_directory(dir);

    std::size_t num = 100;
    std::vector<std::int32_t> values(num);
    std::iota(values.begin(), values.end(), -50);

    {
        H5::H5File handle(dir / "foo.h5", H5F_ACC_TRUNC);
        add_hdf5_numeric_dataset(handle, "bar", H5::PredType::NATIVE_INT32, values);
    }

    H5::H5File handle(dir / "foo.h5", H5F_ACC_RDONLY);
    auto dhandle = handle.openDataSet("bar");

    ritsuko::hdf5::Stream1dNumericDatasetOptions opt;
    opt.contiguous_chunk_size = GetParam();
    takane::NumericStreamIterator<std::int32_t> it(ritsuko::hdf5::Stream1dNumericDataset<std::int32_t>(&dhandle, values.size(), opt));

    std::vector<std::int32_t> output(values.size());
    for (std::size_t i = 0; i < num; ++i) {
        output[i] = it.next();
    }

    EXPECT_EQ(output, values);
}

INSTANTIATE_TEST_SUITE_P(
    IterateStream,
    IterateStreamTest,
    ::testing::Values(3, 30, 300) // chunk sizes
); 

/********************************************/

TEST(ValidateMcols, Okay) {
    auto dir = define_test_path("utils_other");
    initialize_directory(dir);

    // No-op if the directory doesn't exist.
    takane::Options opts;
    std::string name = "mcols";
    takane::validate_mcols(dir, name, 10, opts);

    // Alright, actually creating the directory.
    {
        mock_data_frame(dir / name, 10, {});
    }
    takane::validate_mcols(dir, name, 10, opts);
}

TEST(ValidateMcols, Error) {
    auto dir = define_test_path("utils_other");
    std::string name = "mcols";

    takane::Options opts;

    {
        initialize_directory(dir);
        mock_data_frame(dir / name, 10, {});
    }
    expect_error(
        "unexpected number of rows",
        [&]() -> void {
            takane::validate_mcols(dir, name, 11, opts);
        }
    );

    // Check that we actually validate the data_frame.
    {
        initialize_directory(dir);
        auto ghandle = mock_data_frame(dir / name, 10, {});
        auto dhandle = ghandle.openGroup("data");
        dhandle.createGroup("0");
    }
    expect_error(
        "more objects",
        [&]() -> void {
            takane::validate_mcols(dir, name, 10, opts);
        }
    );

    {
        initialize_directory_simple(dir / name, "simple_list", "1.0");
    }
    expect_error(
        "DATA_FRAME",
        [&]() -> void {
            takane::validate_mcols(dir, name, 10, opts);
        }
    );
}

TEST(ValidateMetadata, Okay) {
    auto dir = define_test_path("utils_other");
    initialize_directory(dir);

    // No-op if the directory doesn't exist.
    takane::Options opts;
    std::string name = "metadata";
    takane::validate_metadata(dir, name, opts);

    {
        mock_simple_list(dir / name);
    }
    takane::validate_metadata(dir, "metadata", opts);
}

TEST(ValidateMetadata, Error) {
    auto dir = define_test_path("utils_other");
    initialize_directory(dir);

    takane::Options opts;
    std::string name = "metadata";

    // Check that we actually validate the simple_list.
    {
        initialize_simple_list_with_metadata(dir / name, "1.0", "json.gz");
        dump_compressed_json(dir / name, "{ \"type\": \"integer\", \"values\": [] }");
    }
    expect_error(
        "top-level",
        [&]() -> void {
            takane::validate_metadata(dir, name, opts);
        }
    );

    {
        mock_data_frame(dir / name, 10, {});
    }
    expect_error(
        "SIMPLE_LIST",
        [&]() -> void {
            takane::validate_metadata(dir, name, opts);
        }
    );
}

TEST(CountDirectoryEntries, Basic) {
    auto dir = define_test_path("utils_other");
    auto path = dir / "counts";
    initialize_directory(path);

    EXPECT_EQ(takane::count_directory_entries(path), 0);

    std::filesystem::create_directory(path / "blah");
    std::filesystem::create_directory(path / "asdasd");
    EXPECT_EQ(takane::count_directory_entries(path), 2);

    // Ignores . and _ prefixes.
    std::filesystem::create_directory(path / "_whee");
    std::filesystem::create_directory(path / ".foo");
    EXPECT_EQ(takane::count_directory_entries(path), 2);
}
