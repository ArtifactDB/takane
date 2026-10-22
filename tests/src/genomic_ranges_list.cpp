#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "utils.h"
#include "mock_genomic_ranges.h"
#include "mock_compressed_list.h"

#include <string>
#include <filesystem>
#include <fstream>

TEST(GenomicRangesList, Okay) {
    auto dir = define_test_path("genomic_ranges_list");

    {
        initialize_directory_simple(dir, "genomic_ranges_list", "1.0");
        mock_compressed_list_partitions(dir / "partitions.h5", "genomic_ranges_list", { 1, 2, 1, 3 });
        mock_genomic_ranges(dir / "concatenated", 7, 3);
    }

    test_validate(dir);
    EXPECT_EQ(test_height(dir), 4);
}

TEST(GenomicRangesList, Error) {
    auto dir = define_test_path("genomic_ranges_list");

    {
        initialize_directory_simple(dir, "genomic_ranges_list", "1.0");
        mock_compressed_list_partitions(dir / "partitions.h5", "genomic_ranges_list", { 1, 2, 1, 3 });
        initialize_directory_simple(dir / "concatenated", "foobar", "1.0");
    }
    expect_validation_error(dir, "'genomic_ranges' type");
}
