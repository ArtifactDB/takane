#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "utils.h"
#include "mock_data_frame.h"
#include "mock_compressed_list.h"

#include "H5Cpp.h"
#include "takane/data_frame_list.hpp"

// These tests are cursory and just check that the correct arguments are passed to validate_compressed_list().
// Most of the heavy testing is actually performed in utils_compressed_list.cpp.

TEST(DataFrameList, Okay) {
    auto dir = define_test_path("data_frame_list");

    {
        initialize_directory_simple(dir, "data_frame_list", "1.0"); 
        mock_compressed_list_partitions(dir / "partitions.h5", "data_frame_list", { 0, 4, 3, 2, 1 });
        std::vector<DataFrameColumnDetails> columns(2);
        columns[0].name = "foo";
        columns[1].name = "bar";
        mock_data_frame(dir / "concatenated", 10, columns);
    }
    test_validate(dir);
    EXPECT_EQ(test_height(dir), 5);
}

TEST(DataFrameList, Error) {
    auto dir = define_test_path("data_frame_list");

    {
        initialize_directory_simple(dir, "data_frame_list", "1.0"); 
        mock_compressed_list_partitions(dir / "partitions.h5", "data_frame_list", { 4, 3, 2, 1 });
        initialize_directory_simple(dir / "concatenated", "foobar", "1.0");
    }
    expect_validation_error(dir, "should satisfy the 'DATA_FRAME' interface");
}
