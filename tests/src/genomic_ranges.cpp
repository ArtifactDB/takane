#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "takane/genomic_ranges.hpp"

#include "utils.h"
#include "mock_sequence_information.h"
#include "mock_genomic_ranges.h"
#include "mock_simple_list.h"
#include "mock_data_frame.h"

#include <fstream>
#include <string>

TEST(GenomicRanges, EndPositionExceedsInt64) {
    EXPECT_FALSE(takane::end_position_exceeds_int64(0, 0));
    EXPECT_FALSE(takane::end_position_exceeds_int64(0, 100));
    EXPECT_FALSE(takane::end_position_exceeds_int64(1, 100));
    EXPECT_FALSE(takane::end_position_exceeds_int64(100, 200));
    EXPECT_FALSE(takane::end_position_exceeds_int64(-100, 200));
    EXPECT_FALSE(takane::end_position_exceeds_int64(-100, 20));

    constexpr std::int64_t max_i64 = std::numeric_limits<std::int64_t>::max();
    constexpr std::int64_t min_i64 = std::numeric_limits<std::int64_t>::lowest();
    constexpr std::uint64_t max_u64 = std::numeric_limits<std::uint64_t>::max();

    EXPECT_FALSE(takane::end_position_exceeds_int64(1, max_i64));
    EXPECT_TRUE(takane::end_position_exceeds_int64(2, max_i64));
    EXPECT_FALSE(takane::end_position_exceeds_int64(2, max_i64 - 1));
    EXPECT_FALSE(takane::end_position_exceeds_int64(-1, static_cast<std::uint64_t>(max_i64) + 2));

    EXPECT_TRUE(takane::end_position_exceeds_int64(1, max_u64));
    EXPECT_TRUE(takane::end_position_exceeds_int64(0, max_u64));
    EXPECT_TRUE(takane::end_position_exceeds_int64(-1, max_u64));

    EXPECT_FALSE(takane::end_position_exceeds_int64(min_i64, max_i64));
    EXPECT_FALSE(takane::end_position_exceeds_int64(min_i64, max_i64 - 1));
    EXPECT_FALSE(takane::end_position_exceeds_int64(min_i64, max_u64));
}

TEST(GenomicRanges, FindSequenceLimits) {
    auto dir = define_test_path("genomic_ranges");
    initialize_directory(dir);

    // No missing values.
    {
        mock_sequence_information(dir, { SequenceInfo("chrA", 100, true, "mm10"), SequenceInfo("chrB", 20, false, "hg19") });
    }
    {
        auto out = takane::find_sequence_limits(dir, {});
        ASSERT_EQ(out.circular.size(), 2);
        ASSERT_EQ(out.length.size(), 2);

        ASSERT_TRUE(out.circular[0].has_value());
        EXPECT_TRUE(*(out.circular[0]));
        ASSERT_TRUE(out.circular[1].has_value());
        EXPECT_FALSE(*(out.circular[1]));

        ASSERT_TRUE(out.length[0].has_value());
        EXPECT_EQ(*(out.length[0]), 100);
        ASSERT_TRUE(out.length[1].has_value());
        EXPECT_EQ(*(out.length[1]), 20);
    }

    // Injecting some missing placeholders.
    {
        auto ghandle = mock_sequence_information(dir, { SequenceInfo("chrA", 100, true, "mm10"), SequenceInfo("chrB", 20, false, "hg19") });
        {
            auto dhandle = ghandle.openDataSet("length");
            auto ahandle = dhandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_UINT32, H5S_SCALAR);
            int val = 20;
            ahandle.write(H5::PredType::NATIVE_INT, &val);
        }
        {
            auto dhandle = ghandle.openDataSet("circular");
            auto ahandle = dhandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_INT8, H5S_SCALAR);
            int val = 1;
            ahandle.write(H5::PredType::NATIVE_INT, &val);
        }
    }
    {
        auto out = takane::find_sequence_limits(dir, {});
        ASSERT_EQ(out.circular.size(), 2);
        ASSERT_EQ(out.length.size(), 2);

        EXPECT_FALSE(out.circular[0].has_value());
        ASSERT_TRUE(out.circular[1].has_value());
        EXPECT_FALSE(*(out.circular[1]));

        ASSERT_TRUE(out.length[0].has_value());
        EXPECT_EQ(*(out.length[0]), 100);
        EXPECT_FALSE(out.length[1].has_value());
    }
}

/********************************/

TEST(GenomicRanges, SimpleOkay) {
    auto dir =  define_test_path("genomic_ranges");

    {
        mock_genomic_ranges(
            dir,
            // Testing intervals around the start and end of each sequence.
            {  
                GenomicRange(2, 12, 20, 1),
                GenomicRange(1, 1, 231, 0),
                GenomicRange(0, 1, 20, -1),
                GenomicRange(0, 52, 48, 0),
                GenomicRange(2, 10, 3, -1),
                GenomicRange(1, 2, 230, 1),
                GenomicRange(0, 34, 66, 1)
            },
            {
                SequenceInfo("akira", 99, false, "animation"),
                SequenceInfo("ai", 231, false, "origination"),
                SequenceInfo("alice", 31, false, "natural")
            }
        );
    }

    test_validate(dir);
    EXPECT_EQ(test_height(dir), 7);
}

TEST(GenomicRanges, MixedOkay) {
    auto dir =  define_test_path("genomic_ranges");

    {
        mock_genomic_ranges(
            dir,
            // Testing intervals around the start and end of each sequence.
            {  
                GenomicRange(2, 2000, 3000, 0),
                GenomicRange(3, 1, 30, -1),
                GenomicRange(1, -10, 2900, 0),
                GenomicRange(2, -1000, 50, 1),
                GenomicRange(0, 1, 20, 1),
                GenomicRange(5, 1000, 6200, 0),
                GenomicRange(1, 10000, 20, -1),
                GenomicRange(4, 290, 3000, 0),
                GenomicRange(0, 1000, 20000, 1),
                GenomicRange(3, 71, 30, 0),
                GenomicRange(5, -1000, 200, 0),
                GenomicRange(4, -10, 200, -1)
            },
            // Testing every combination of circular/non-circular/unknown with known/unknown length.
            // We'll use zero as a missing value placeholder for length.
            {
                SequenceInfo("athena", 0, false, "animation"),
                SequenceInfo("aika", 0, true, "origination"),
                SequenceInfo("alicia", 0, -1, "natural"),
                SequenceInfo("akari", 100, false, "natural"),
                SequenceInfo("alice", 300, true, "animation"),
                SequenceInfo("akira", 200, -1, "origination")
            }
        );

        H5::H5File handle(dir / "sequence_information" / "info.h5", H5F_ACC_RDWR);
        auto ghandle = handle.openGroup("sequence_information");
        auto lhandle = ghandle.openDataSet("length");
        add_hdf5_numeric_attribute<std::uint32_t>(lhandle, "missing-value-placeholder", 0);
        auto chandle = ghandle.openDataSet("circular");
        add_hdf5_numeric_attribute<std::int8_t>(chandle, "missing-value-placeholder", -1);
    }

    test_validate(dir);
    EXPECT_EQ(test_height(dir), 12);
}

/********************************/

TEST(GenomicRanges, VersionError) {
    auto dir =  define_test_path("genomic_ranges");

    {
        initialize_directory_simple(dir, "genomic_ranges", "2.0");
    }
    expect_validation_error(dir, "unsupported version");
}

TEST(GenomicRanges, SequenceError) {
    auto dir =  define_test_path("genomic_ranges");

    {
        auto ghandle = mock_genomic_ranges(dir, {}, {});
        ghandle.unlink("sequence");
        add_hdf5_dataset(ghandle, "sequence", H5::PredType::NATIVE_INT32, 0);
    }
    expect_validation_error(dir, "64-bit unsigned integer");

    {
        auto ghandle = mock_genomic_ranges(dir, {}, {});
        ghandle.unlink("sequence");
        ghandle.createDataSet("sequence", H5::PredType::NATIVE_UINT32, H5S_SCALAR);
    }
    expect_validation_error(dir, "1-dimensional");
}

TEST(GenomicRanges, StartError) {
    auto dir =  define_test_path("genomic_ranges");

    {
        auto ghandle = mock_genomic_ranges(dir, { GenomicRange(0, 1, 20, 0) }, { SequenceInfo("foo", 30, 0, "mm10") });
        ghandle.unlink("start");
        add_hdf5_dataset(ghandle, "start", H5::PredType::NATIVE_UINT64, 1);
    }
    expect_validation_error(dir, "64-bit signed integer");

    {
        auto ghandle = mock_genomic_ranges(dir, { GenomicRange(0, 1, 20, 0) }, { SequenceInfo("foo", 30, 0, "mm10") });
        ghandle.unlink("start");
        ghandle.createDataSet("start", H5::PredType::NATIVE_INT32, H5S_SCALAR);
    }
    expect_validation_error(dir, "1-dimensional");

    {
        auto ghandle = mock_genomic_ranges(dir, { GenomicRange(0, 1, 20, 0) }, { SequenceInfo("foo", 30, 0, "mm10") });
        ghandle.unlink("start");
        add_hdf5_dataset(ghandle, "start", H5::PredType::NATIVE_INT64, 2);
    }
    expect_validation_error(dir, "same as that of 'sequence'");
}

TEST(GenomicRanges, WidthError) {
    auto dir =  define_test_path("genomic_ranges");

    {
        auto ghandle = mock_genomic_ranges(dir, { GenomicRange(0, 1, 20, 0) }, { SequenceInfo("foo", 30, 0, "mm10") });
        ghandle.unlink("width");
        add_hdf5_dataset(ghandle, "width", H5::PredType::NATIVE_INT32, 1);
    }
    expect_validation_error(dir, "64-bit unsigned integer");

    {
        auto ghandle = mock_genomic_ranges(dir, { GenomicRange(0, 1, 20, 0) }, { SequenceInfo("foo", 30, 0, "mm10") });
        ghandle.unlink("width");
        ghandle.createDataSet("width", H5::PredType::NATIVE_UINT32, H5S_SCALAR);
    }
    expect_validation_error(dir, "1-dimensional");

    {
        auto ghandle = mock_genomic_ranges(dir, { GenomicRange(0, 1, 20, 0) }, { SequenceInfo("foo", 30, 0, "mm10") });
        ghandle.unlink("width");
        add_hdf5_dataset(ghandle, "width", H5::PredType::NATIVE_UINT64, 0);
    }
    expect_validation_error(dir, "same as that of 'sequence'");
}

TEST(GenomicRanges, StrandError) {
    auto dir =  define_test_path("genomic_ranges");

    {
        auto ghandle = mock_genomic_ranges(dir, { GenomicRange(0, 1, 20, 0) }, { SequenceInfo("foo", 30, 0, "mm10") });
        ghandle.unlink("strand");
        add_hdf5_dataset(ghandle, "strand", H5::PredType::NATIVE_UINT32, 1);
    }
    expect_validation_error(dir, "32-bit signed integer");

    {
        auto ghandle = mock_genomic_ranges(dir, { GenomicRange(0, 1, 20, 0) }, { SequenceInfo("foo", 30, 0, "mm10") });
        ghandle.unlink("strand");
        ghandle.createDataSet("strand", H5::PredType::NATIVE_INT32, H5S_SCALAR);
    }
    expect_validation_error(dir, "1-dimensional");

    {
        auto ghandle = mock_genomic_ranges(dir, { GenomicRange(0, 1, 20, 0) }, { SequenceInfo("foo", 30, 0, "mm10") });
        ghandle.unlink("strand");
        add_hdf5_dataset(ghandle, "strand", H5::PredType::NATIVE_INT32, 3);
    }
    expect_validation_error(dir, "same as that of 'sequence'");

    {
        mock_genomic_ranges(dir, { GenomicRange(0, 1, 20, 2), GenomicRange(0, 1, 20, -1), }, { SequenceInfo("foo", 30, 0, "mm10") });
    }
    expect_validation_error(dir, "entries should be one of");

    {
        mock_genomic_ranges(dir, { GenomicRange(0, 1, 20, 0), GenomicRange(0, 1, 20, -2) }, { SequenceInfo("foo", 30, 0, "mm10") });
    }
    expect_validation_error(dir, "entries should be one of");
}

/********************************/

TEST(GenomicRanges, IntervalSequenceError) {
    auto dir =  define_test_path("genomic_ranges");

    {
        mock_genomic_ranges(
            dir,
            {
                GenomicRange(0, 1, 20, 0),
                GenomicRange(1, 2, 30, -1),
                GenomicRange(2, 3, 40, 1) // fail
            },
            {
                SequenceInfo("foo", 40, false, "hg38"),
                SequenceInfo("bar", 500, false, "hg38")
            }
        );
    }
    expect_validation_error(dir, "less than the number of sequences");
}

TEST(GenomicRanges, IntervalStartError) {
    auto dir =  define_test_path("genomic_ranges");

    {
        mock_genomic_ranges(
            dir,
            {
                GenomicRange(0, 1, 20, 0),
                GenomicRange(1, 0, 30, -1), // fail
                GenomicRange(0, 3, 30, 1)
            },
            {
                SequenceInfo("foo", 40, false, "hg38"),
                SequenceInfo("bar", 500, false, "hg38")
            }
        );
    }
    expect_validation_error(dir, "non-positive");

    {
        mock_genomic_ranges(
            dir,
            {
                GenomicRange(0, 1, 20, 0),
                GenomicRange(1, 100, 30, -1),
                GenomicRange(0, -20, 30, 1) // fail
            },
            {
                SequenceInfo("foo", 40, false, "hg38"),
                SequenceInfo("bar", 500, false, "hg38")
            }
        );
    }
    expect_validation_error(dir, "non-positive");

    {
        mock_genomic_ranges(
            dir,
            {
                GenomicRange(0, 100, 20, 0), // fail
                GenomicRange(1, 100, 30, -1),
                GenomicRange(0, 10, 30, 1)
            },
            {
                SequenceInfo("foo", 40, false, "hg38"),
                SequenceInfo("bar", 500, false, "hg38")
            }
        );
    }
    expect_validation_error(dir, "'start' position exceeds sequence length");
}

TEST(GenomicRanges, IntervalEndError) {
    auto dir =  define_test_path("genomic_ranges");

    {
        mock_genomic_ranges(
            dir,
            {
                GenomicRange(0, 15, 20, 0),
                GenomicRange(1, 100, 30, -1),
                GenomicRange(0, 10, 32, 1) // fail
            },
            {
                SequenceInfo("foo", 40, false, "hg38"),
                SequenceInfo("bar", 500, false, "hg38")
            }
        );
    }
    expect_validation_error(dir, "end position");

    {
        {
            auto ghandle = mock_genomic_ranges(
                dir,
                {
                    GenomicRange(1, 2, 20, 0),
                    GenomicRange(0, 5, 30, -1),
                    GenomicRange(1, 8, 32, 1)
                },
                {
                    SequenceInfo("foo", 40, false, "hg38"),
                    SequenceInfo("bar", 500, false, "hg38")
                }
            );
            ghandle.unlink("start");
            auto dhandle = add_hdf5_dataset(ghandle, "start", H5::PredType::NATIVE_INT64, 3);
            std::vector<std::int64_t> content { std::numeric_limits<std::int64_t>::max(), std::int64_t(5), std::int64_t(8) };
            dhandle.write(content.data(), H5::PredType::NATIVE_INT64);
        }
        {
            H5::H5File s_handle(dir / "sequence_information" / "info.h5", H5F_ACC_RDWR);
            auto s_ghandle = s_handle.openGroup("sequence_information");
            s_ghandle.unlink("length");
            auto s_lhandle = add_hdf5_dataset(s_ghandle, "length", H5::PredType::NATIVE_UINT64, 2);
            std::vector<std::uint64_t> content { std::numeric_limits<std::uint64_t>::max(), std::numeric_limits<std::uint64_t>::max() };
            s_lhandle.write(content.data(), H5::PredType::NATIVE_UINT64);
        }
    }
    expect_validation_error(dir, "beyond the range");
}

/********************************/

static H5::Group quick_mock(const std::filesystem::path& dir) {
    return mock_genomic_ranges(
        dir,
        {  
            GenomicRange(0, 10, 500, 1),
            GenomicRange(2, 1, 99, 0),
            GenomicRange(1, 1, 900, -1),
            GenomicRange(0, 52, 47, 0),
            GenomicRange(1, 10, 800, -1),
            GenomicRange(1, 1, 99, 1),
            GenomicRange(2, 330, 100, 0),
            GenomicRange(2, 1, 500, 1)
        },
        {
            SequenceInfo("kanon", 1000, false, "liella"),
            SequenceInfo("chisato", 900, false, "liella"),
            SequenceInfo("keke", 500, false, "liella")
        }
    );
}

TEST(GenomicRanges, NamesOkay) {
    auto dir = define_test_path("genomic_ranges");

    {
        auto ghandle = quick_mock(dir);
        add_hdf5_dataset(ghandle, "name", H5::StrType(0, 5), 8);
    }

    test_validate(dir);
    EXPECT_EQ(test_height(dir), 8);
}

TEST(GenomicRanges, NamesError) {
    auto dir = define_test_path("genomic_ranges");

    {
        auto ghandle = quick_mock(dir);
        add_hdf5_dataset(ghandle, "name", H5::StrType(0, 5), 3);
    }
    expect_validation_error(dir, "number of names");
}

TEST(GenomicRanges, McolsOkay) {
    auto dir = define_test_path("genomic_ranges");

    {
        quick_mock(dir);
        std::vector<DataFrameColumnDetails> cols(2);
        cols[0].name = "year";
        cols[1].name = "height";
        mock_data_frame(dir / "range_annotations", 8, cols);
    }

    test_validate(dir);
    EXPECT_EQ(test_height(dir), 8);
}

TEST(GenomicRanges, McolsOError) {
    auto dir = define_test_path("genomic_ranges");

    {
        quick_mock(dir);
        std::vector<DataFrameColumnDetails> cols(2);
        cols[0].name = "age";
        cols[1].name = "blood_type";
        mock_data_frame(dir / "range_annotations", 9, cols);
    }

    expect_validation_error(dir, "number of rows");
}

TEST(GenomicRanges, MetadataOkay) {
    auto dir = define_test_path("genomic_ranges");

    {
        quick_mock(dir);
        mock_simple_list(dir / "other_annotations");
    }

    test_validate(dir);
    EXPECT_EQ(test_height(dir), 8);
}

TEST(GenomicRanges, MetadataError) {
    auto dir = define_test_path("genomic_ranges");

    {
        quick_mock(dir);
        mock_data_frame(dir / "other_annotations", 8, {});
    }

    expect_validation_error(dir, "'SIMPLE_LIST'");
}
