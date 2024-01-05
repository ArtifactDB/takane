#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "takane/genomic_ranges.hpp"

#include "utils.h"
#include "sequence_information.h"
#include "genomic_ranges.h"
#include "simple_list.h"
#include "data_frame.h"

#include <fstream>
#include <string>

struct GenomicRangesTest : public ::testing::Test {
    GenomicRangesTest() {
        dir = "TEST_genomic_ranges";
        name = "genomic_ranges";
    }

    std::filesystem::path dir;
    std::string name;

    H5::H5File reopen() {
        return H5::H5File(dir / "ranges.h5", H5F_ACC_RDWR);
    }

    template<typename ... Args_>
    void expect_error(const std::string& msg, Args_&& ... args) {
        EXPECT_ANY_THROW({
            try {
                test_validate(dir, std::forward<Args_>(args)...);
            } catch (std::exception& e) {
                EXPECT_THAT(e.what(), ::testing::HasSubstr(msg));
                throw;
            }
        });
    }
};

TEST_F(GenomicRangesTest, SeqInfoRetrieval) {
    auto sidir = dir / "sequence_information";

    {
        initialize_directory_simple(dir, "genomic_ranges", "1.0");
        initialize_directory_simple(sidir, "sequence_information", "1.0");
        H5::H5File handle(sidir / "info.h5", H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup("sequence_information");
        sequence_information::mock(ghandle, { "chrA", "chrB" }, { 100, 20 }, { 1, 0 }, { "mm10", "mm10 "});
    }
    {
        auto out = takane::genomic_ranges::internal::find_sequence_limits(sidir, takane::Options());
        EXPECT_EQ(out.has_circular, std::vector<unsigned char>(2, true));
        EXPECT_EQ(out.has_seqlen, std::vector<unsigned char>(2, true));
        std::vector<unsigned char> expected_circular{ 1, 0 };
        EXPECT_EQ(out.circular, expected_circular);
        std::vector<uint64_t> expected_seqlen { 100, 20 };
        EXPECT_EQ(out.seqlen, expected_seqlen);
    }

    // Injecting some missing values.
    {
        H5::H5File handle(sidir / "info.h5", H5F_ACC_RDWR);
        auto ghandle = handle.openGroup("sequence_information");
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
        auto out = takane::genomic_ranges::internal::find_sequence_limits(sidir, takane::Options());
        std::vector<unsigned char> has_circular{0, 1};
        EXPECT_EQ(out.has_circular, has_circular);
        std::vector<unsigned char> has_seqlen{0, 1};
        std::vector<unsigned char> expected_circular{ 1, 0 };
        EXPECT_EQ(out.circular, expected_circular);
        std::vector<uint64_t> expected_seqlen { 100, 20 };
        EXPECT_EQ(out.seqlen, expected_seqlen);
    }

    // Shifting the placeholders to be ineffective.
    {
        H5::H5File handle(sidir / "info.h5", H5F_ACC_RDWR);
        auto ghandle = handle.openGroup("sequence_information");
        {
            auto dhandle = ghandle.openDataSet("length");
            dhandle.removeAttr("missing-value-placeholder");
            auto ahandle = dhandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_UINT32, H5S_SCALAR);
            int val = 10;
            ahandle.write(H5::PredType::NATIVE_INT, &val);
        }
        {
            auto dhandle = ghandle.openDataSet("circular");
            dhandle.removeAttr("missing-value-placeholder");
            auto ahandle = dhandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_INT8, H5S_SCALAR);
            int val = -1;
            ahandle.write(H5::PredType::NATIVE_INT, &val);
        }
    }
    {
        auto out = takane::genomic_ranges::internal::find_sequence_limits(sidir, takane::Options());
        EXPECT_EQ(out.has_circular, std::vector<unsigned char>(2, true));
        EXPECT_EQ(out.has_seqlen, std::vector<unsigned char>(2, true));
        std::vector<unsigned char> expected_circular{ 1, 0 };
        EXPECT_EQ(out.circular, expected_circular);
        std::vector<uint64_t> expected_seqlen { 100, 20 };
        EXPECT_EQ(out.seqlen, expected_seqlen);
    }
}

TEST_F(GenomicRangesTest, BasicChecks) {
    initialize_directory_simple(dir, name, "2.0");
    expect_error("unsupported version");

    genomic_ranges::mock(dir, 58, 13);
    test_validate(dir);
    EXPECT_EQ(test_height(dir), 58);

    auto sidir = dir / "sequence_information";
    initialize_directory_simple(sidir, "FOOBAR", "1.0");
    expect_error("'sequence_information' object");
}

TEST_F(GenomicRangesTest, Sequence) {
    genomic_ranges::mock(dir, 
        { 0, 1, 0, 2 },
        { 1, 5, 10, 20 },
        { 10, 5, 10, 50 },
        { 1, -1, 0, -1 },
        { 100, 200, 300 },
        { 0, 0, 0 }
    );

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup("genomic_ranges");
        ghandle.unlink("sequence");
        hdf5_utils::spawn_data(ghandle, "sequence", 4, H5::PredType::NATIVE_INT32);
    }
    expect_error("64-bit unsigned integer");

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup("genomic_ranges");
        ghandle.unlink("sequence");
        hdf5_utils::spawn_numeric_data<int>(ghandle, "sequence", H5::PredType::NATIVE_UINT32, { 0, 1, 2, 5 });
    }
    expect_error("less than the number of sequences");
}

TEST_F(GenomicRangesTest, Start) {
    genomic_ranges::mock(dir, 
        { 0, 1, 0, 2 },
        { 1, 10, 20 },
        { 10, 5, 10, 50 },
        { 1, -1, 0, -1 },
        { 100, 200, 300 },
        { 0, 0, 0 }
    );
    expect_error("same length");

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup("genomic_ranges");
        ghandle.unlink("start");
        hdf5_utils::spawn_data(ghandle, "start", 4, H5::PredType::NATIVE_UINT64);
    }
    expect_error("64-bit signed integer");
}

TEST_F(GenomicRangesTest, Width) {
    genomic_ranges::mock(dir, 
        { 0, 1, 0, 2 },
        { 1, 10, 20, 50 },
        { 10, 5, 10 },
        { 1, -1, 0, -1 },
        { 100, 200, 300 },
        { 0, 0, 0 }
    );
    expect_error("same length");

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup("genomic_ranges");
        ghandle.unlink("width");
        hdf5_utils::spawn_data(ghandle, "width", 4, H5::PredType::NATIVE_INT64);
    }
    expect_error("64-bit unsigned integer");
}

TEST_F(GenomicRangesTest, Int64Ends) {
    genomic_ranges::mock(dir, 
        { 0, 1, 0, 2 },
        {},
        {},
        { 1, -1, 0, -1 },
        { 100, 200, 300 },
        { 1, 1, 1 }
    );

    constexpr int64_t max_i64 = std::numeric_limits<int64_t>::max();
    constexpr int64_t min_i64 = std::numeric_limits<int64_t>::min();
    constexpr uint64_t max_u64 = std::numeric_limits<uint64_t>::max();

    // Checking that all extremes pass.
    {
        auto handle = reopen();
        auto ghandle = handle.openGroup("genomic_ranges");
        ghandle.unlink("start");
        hdf5_utils::spawn_numeric_data<int64_t>(ghandle, "start", H5::PredType::NATIVE_INT64, { max_i64, -100, 0, min_i64 });
        ghandle.unlink("width");
        hdf5_utils::spawn_numeric_data<uint64_t>(ghandle, "width", H5::PredType::NATIVE_UINT64, { 0, 1, static_cast<uint64_t>(max_i64), max_u64 });
    }
    test_validate(dir);

    // Failing when start is positive.
    {
        auto handle = reopen();
        auto ghandle = handle.openGroup("genomic_ranges");
        ghandle.unlink("width");
        hdf5_utils::spawn_numeric_data<uint64_t>(ghandle, "width", H5::PredType::NATIVE_UINT64, { 1, 1, static_cast<uint64_t>(max_i64), max_u64 });
    }
    expect_error("beyond the range of a 64-bit");

    // Failing when start is negative.
    {
        auto handle = reopen();
        auto ghandle = handle.openGroup("genomic_ranges");
        ghandle.unlink("width");
        hdf5_utils::spawn_numeric_data<uint64_t>(ghandle, "width", H5::PredType::NATIVE_UINT64, { 0, max_u64, static_cast<uint64_t>(max_i64), max_u64 });
    }
    expect_error("beyond the range of a 64-bit");
}

TEST_F(GenomicRangesTest, RestrictedStart) {
    genomic_ranges::mock(dir, 
        { 0, 1, 0, 2 },
        { 90, 100, 50, 100 },
        { 11, 101, 51, 201 },
        { 1, -1, 0, -1 },
        { 100, 200, 300 },
        { 0, 0, 0 }
    );
    test_validate(dir);

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup("genomic_ranges");
        ghandle.unlink("start");
        hdf5_utils::spawn_numeric_data<int64_t>(ghandle, "start", H5::PredType::NATIVE_UINT32, { 90, 100, 0, 100 }); // The third interval now starts before position 1.
    }
    expect_error("non-positive");

    auto tmp_path = dir / "sequence_information" / "_temp.h5";
    auto info_path = dir / "sequence_information" / "info.h5";
    std::filesystem::copy(info_path, tmp_path);

    {
        H5::H5File handle(info_path, H5F_ACC_RDWR);
        auto ghandle = handle.openGroup("sequence_information");
        auto dhandle = ghandle.openDataSet("circular");
        auto ahandle = dhandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_INT8, H5S_SCALAR); // rescuing it by making all the circular flags missing.
        int val = 0;
        ahandle.write(H5::PredType::NATIVE_INT, &val);
    }
    test_validate(dir);

    {
        std::filesystem::copy(tmp_path, info_path, std::filesystem::copy_options::update_existing);
        H5::H5File handle(info_path, H5F_ACC_RDWR);
        auto ghandle = handle.openGroup("sequence_information");
        ghandle.unlink("circular");
        hdf5_utils::spawn_numeric_data<int32_t>(ghandle, "circular", H5::PredType::NATIVE_INT32, { 1, 0, 0 }); // rescuing it by making the first sequence circular.
    }
    test_validate(dir);
}

TEST_F(GenomicRangesTest, RestrictedEnd) {
    genomic_ranges::mock(dir, 
        { 0, 1, 0, 2 },
        { 90, 100, 50, 100 },
        { 11, 101, 51, 201 },
        { 1, -1, 0, -1 },
        { 100, 200, 300 },
        { 0, 0, 0 }
    );

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup("genomic_ranges");
        ghandle.unlink("width");
        hdf5_utils::spawn_numeric_data<uint64_t>(ghandle, "width", H5::PredType::NATIVE_UINT64, { 11, 101, 52, 201 }); // extending the third interval beyond the end.
    }
    expect_error("end position beyond sequence length");

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup("genomic_ranges");
        ghandle.unlink("start");
        ghandle.unlink("width");
        hdf5_utils::spawn_numeric_data<uint64_t>(ghandle, "width", H5::PredType::NATIVE_UINT64, { 11, 101, 51, 201 }); // restoring the third interval's length.
        hdf5_utils::spawn_numeric_data<int64_t>(ghandle, "start", H5::PredType::NATIVE_UINT32, {90, 300, 50, 100}); // shifting the second interval beyond the end.
    }
    expect_error("start position beyond sequence length");

    auto tmp_path = dir / "sequence_information" / "_temp.h5";
    auto info_path = dir / "sequence_information" / "info.h5";
    std::filesystem::copy(info_path, tmp_path);

    {
        H5::H5File handle(info_path, H5F_ACC_RDWR);
        auto ghandle = handle.openGroup("sequence_information");
        ghandle.unlink("circular");
        hdf5_utils::spawn_numeric_data<int32_t>(ghandle, "circular", H5::PredType::NATIVE_INT32, { 0, 1, 0 }); // rescuing it by making the second sequence circular.
    }
    test_validate(dir);

    {
        std::filesystem::copy(tmp_path, info_path, std::filesystem::copy_options::update_existing);
        H5::H5File handle(info_path, H5F_ACC_RDWR);
        auto ghandle = handle.openGroup("sequence_information");
        auto dhandle = ghandle.openDataSet("length");
        auto ahandle = dhandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_UINT32, H5S_SCALAR); // rescuing it by making the second sequence length missing.
        int val = 200;
        ahandle.write(H5::PredType::NATIVE_INT, &val);
    }
    test_validate(dir);

    {
        std::filesystem::copy(tmp_path, info_path, std::filesystem::copy_options::update_existing);
        H5::H5File handle(info_path, H5F_ACC_RDWR);
        auto ghandle = handle.openGroup("sequence_information");
        ghandle.unlink("circular");
        auto dhandle = hdf5_utils::spawn_numeric_data<int32_t>(ghandle, "circular", H5::PredType::NATIVE_INT8, { 0, 0, 0 }); 
        auto ahandle = dhandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_INT8, H5S_SCALAR); // rescuing it by making all the circular flags missing.
        int val = 0;
        ahandle.write(H5::PredType::NATIVE_INT, &val);
    }
    test_validate(dir);
}

TEST_F(GenomicRangesTest, Strand) {
    genomic_ranges::mock(dir, 
        { 0, 1, 0, 2 },
        { 90, 100, 50, 100 },
        { 11, 101, 51, 201 },
        { 1, -1, 0, 2 },
        { 100, 200, 300 },
        { 0, 0, 0 }
    );
    expect_error("0, -1, or 1");

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup("genomic_ranges");
        ghandle.unlink("strand");
        hdf5_utils::spawn_data(ghandle, "strand", 4, H5::PredType::NATIVE_UINT32);
    }
    expect_error("32-bit signed integer");

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup("genomic_ranges");
        ghandle.unlink("strand");
        hdf5_utils::spawn_data(ghandle, "strand", 3, H5::PredType::NATIVE_INT32);
    }
    expect_error("same length");
}

TEST_F(GenomicRangesTest, Names) {
    genomic_ranges::mock(dir, 6, 4); 

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup("genomic_ranges");
        hdf5_utils::spawn_string_data(ghandle, "name", 2, { "A", "B", "C", "D", "E", "F" });
    }
    test_validate(dir);

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup("genomic_ranges");
        ghandle.unlink("name");
        hdf5_utils::spawn_string_data(ghandle, "name", 2, { "A", "B", "C" });
    }
    expect_error("same length as");
}

TEST_F(GenomicRangesTest, Metadata) {
    genomic_ranges::mock(dir, 25, 11); 

    auto odir = dir / "other_annotations";
    auto rdir = dir / "range_annotations";

    initialize_directory_simple(rdir, "simple_list", "1.0");
    expect_error("'DATA_FRAME'"); 

    data_frame::mock(rdir, 25, {});
    initialize_directory_simple(odir, "data_frame", "1.0");
    expect_error("'SIMPLE_LIST'");

    simple_list::mock(odir);
    test_validate(dir);
}
