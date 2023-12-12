#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "takane/takane.hpp"
#include "utils.h"
#include "genomic_ranges.h"
#include "data_frame.h"
#include "simple_list.h"

#include <string>
#include <filesystem>
#include <fstream>

struct GenomicRangesListTest : public::testing::Test {
    GenomicRangesListTest() {
        dir = "TEST_genomic_ranges_list";
        name = "genomic_ranges_list";
    }

    std::filesystem::path dir;
    std::string name;

    H5::H5File initialize() {
        initialize_directory_simple(dir, name, "1.0");
        return H5::H5File(dir / "partitions.h5", H5F_ACC_TRUNC);
    }

    H5::H5File reopen() {
        return H5::H5File(dir / "partitions.h5", H5F_ACC_RDWR);
    }

    void expect_error(const std::string& msg) {
        EXPECT_ANY_THROW({
            try {
                takane::validate(dir);
            } catch (std::exception& e) {
                EXPECT_THAT(e.what(), ::testing::HasSubstr(msg));
                throw;
            }
        });
    }
};

TEST_F(GenomicRangesListTest, Basic) {
    initialize_directory_simple(dir, name, "2.0");
    expect_error("unsupported version string");

    {
        auto handle = initialize();
        auto ghandle = handle.createGroup(name);
        hdf5_utils::spawn_numeric_data<int>(ghandle, "lengths", H5::PredType::NATIVE_UINT32, { 1, 2, 1, 3 });
        initialize_directory_simple(dir / "concatenated", "foobar", "1.0");
    }
    expect_error("'genomic_ranges'");

    {
        initialize_directory_simple(dir / "concatenated", "genomic_ranges", "1.0");
    }
    expect_error("failed to validate the 'concatenated'");

    {
        genomic_ranges::mock(dir / "concatenated", 8, 5);
    }
    expect_error("sum of 'lengths'");

    {
        genomic_ranges::mock(dir / "concatenated", 7, 3);
    }
    takane::validate(dir);
    EXPECT_EQ(takane::height(dir), 4);
}
