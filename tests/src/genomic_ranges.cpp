#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "takane/takane.hpp"
#include "sequence_information.h"

#include <fstream>
#include <string>

struct GenomicRangesTest : public ::testing::Test {
    GenomicRangesTest() {
        dir = "TEST_genomic_ranges";
        name = "genomic_ranges";
    }

    std::filesystem::path dir;
    std::string name;

    H5::H5File initialize() {
        initialize_directory(dir, name);
        auto path = dir / "ranges.h5";
        return H5::H5File(std::string(path), H5F_ACC_TRUNC);
    }
};

TEST_F(GenomicRangesTest, SeqInfoRetrieval) {
    auto sidir = dir / "sequence_information";

    {
        initialize();
        initialize_directory(sidir, "sequence_information");
        H5::H5File handle(sidir / "info.h5", H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup("sequence_information");
        sequence_information::mock(ghandle, { "chrA", "chrB" }, { 100, 20 }, { 1, 0 }, { "mm10", "mm10 "});
    }
    {
        auto out = takane::genomic_ranges::internal::find_sequence_limits(sidir, takane::Options());
        std::vector<unsigned char> expected_restricted { 0, 1 };
        EXPECT_EQ(out.restricted, expected_restricted);
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
        std::vector<unsigned char> expected_restricted { 1, 0 };
        EXPECT_EQ(out.restricted, expected_restricted);
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
        std::vector<unsigned char> expected_restricted { 0, 1 };
        EXPECT_EQ(out.restricted, expected_restricted);
    }
}

//TEST(GenomicRanges, Names) {
//    std::string buffer = "\"names\",\"seqnames\",\"start\",\"end\",\"strand\"\n";
//    buffer += "\"foo\",\"chrA\",4,10,\"*\"\n";
//    buffer += "\"bar\",\"chrB\",9,9,\"+\"\n";
//    buffer += "\"whee\",\"chrC\",19,100,\"-\"\n";
//    validate(buffer, 3, true, std::unordered_set<std::string>{ "chrA", "chrB", "chrC" });
//
//    expect_error("number of fields", buffer, 3, false, std::unordered_set<std::string>{ "chrA", "chrB", "chrC" }); 
//    expect_error("number of records", buffer, 10, true, std::unordered_set<std::string>{ "chrA", "chrB", "chrC" }); 
//
//    buffer = "\"seqnames\",\"start\",\"end\",\"strand\"\n";
//    buffer += "\"chrA\",4,10,\"*\"\n";
//    buffer += "\"chrB\",9,9,\"+\"\n";
//    buffer += "\"chrC\",19,100,\"-\"\n";
//    validate(buffer, 3, false, std::unordered_set<std::string>{ "chrA", "chrB", "chrC" }); 
//
//    buffer = "\"names\",\"seqnames\",\"start\",\"end\",\"strand\"\n";
//    buffer += "NA,\"chrA\",4,10,\"*\"\n";
//    buffer += "NA,\"chrB\",9,9,\"+\"\n";
//    buffer += "NA,\"chrC\",19,100,\"-\"\n";
//    expect_error("missing values", buffer, 3, true, std::unordered_set<std::string>{ "chrA", "chrB", "chrC" }); 
//}

//TEST(GenomicRanges, Seqnames) {
//    std::string buffer = "\"seqnames\",\"start\",\"end\",\"strand\"\n";
//    buffer += "\"chrA\",4,10,\"*\"\n";
//    buffer += "\"chrB\",9,9,\"+\"\n";
//    buffer += "\"chrC\",19,100,\"-\"\n";
//    expect_error("unknown sequence name", buffer, 3, false, std::unordered_set<std::string>{ "chrA", "chrB" }); 
//
//    buffer = "\"seqnames\",\"start\",\"end\",\"strand\"\n";
//    buffer += "NA,4,10,\"*\"\n";
//    expect_error("missing values", buffer, 1, false, std::unordered_set<std::string>{ "chrA" }); 
//}
//
//TEST(GenomicRanges, Start) {
//    std::string buffer = "\"seqnames\",\"start\",\"end\",\"strand\"\n";
//    buffer += "\"chrA\",4.5,10,\"*\"\n";
//    expect_error("is not an integer", buffer, 1, false, std::unordered_set<std::string>{ "chrA", "chrB" }); 
//
//    buffer = "\"seqnames\",\"start\",\"end\",\"strand\"\n";
//    buffer += "\"chrA\",3000000000,10,\"*\"\n";
//    expect_error("does not fit", buffer, 1, false, std::unordered_set<std::string>{ "chrA", "chrB" }); 
//
//    buffer = "\"seqnames\",\"start\",\"end\",\"strand\"\n";
//    buffer += "\"chrA\",NA,10,\"*\"\n";
//    expect_error("missing value", buffer, 1, false, std::unordered_set<std::string>{ "chrA", "chrB" }); 
//}
//
//TEST(GenomicRanges, End) {
//    std::string buffer = "\"seqnames\",\"start\",\"end\",\"strand\"\n";
//    buffer += "\"chrA\",4,5.1,\"*\"\n";
//    expect_error("is not an integer", buffer, 1, false, std::unordered_set<std::string>{ "chrA", "chrB" }); 
//
//    buffer = "\"seqnames\",\"start\",\"end\",\"strand\"\n";
//    buffer += "\"chrA\",10,3000000000,\"*\"\n";
//    expect_error("does not fit", buffer, 1, false, std::unordered_set<std::string>{ "chrA", "chrB" }); 
//
//    buffer = "\"seqnames\",\"start\",\"end\",\"strand\"\n";
//    buffer += "\"chrA\",10,NA,\"*\"\n";
//    expect_error("missing value", buffer, 1, false, std::unordered_set<std::string>{ "chrA", "chrB" }); 
//
//    buffer = "\"seqnames\",\"start\",\"end\",\"strand\"\n";
//    buffer += "\"chrB\",99,108,\"*\"\n";
//    buffer += "\"chrA\",10,8,\"*\"\n";
//    buffer += "\"chrB\",50,100,\"*\"\n";
//    expect_error("greater than or equal to", buffer, 3, false, std::unordered_set<std::string>{ "chrA", "chrB" }); 
//
//    buffer = "\"seqnames\",\"start\",\"end\",\"strand\"\n";
//    buffer += "\"chrA\",10,9,\"*\"\n";
//    validate(buffer, 1, false, std::unordered_set<std::string>{ "chrA", "chrB", "chrC" });
//}
//
//TEST(GenomicRanges, Strand) {
//    std::string buffer = "\"seqnames\",\"start\",\"end\",\"strand\"\n";
//    buffer += "\"chrA\",4,5,NA\n";
//    expect_error("missing value", buffer, 1, false, std::unordered_set<std::string>{ "chrA", "chrB" }); 
//
//    buffer = "\"seqnames\",\"start\",\"end\",\"strand\"\n";
//    buffer += "\"chrA\",4,51,\"5\"\n";
//    expect_error("invalid strand", buffer, 1, false, std::unordered_set<std::string>{ "chrA", "chrB" }); 
//}
//
//TEST(GenomicRanges, ColumnNames) {
//    std::string buffer = "\"foo\",\"start\",\"end\",\"strand\"\n";
//    buffer += "\"chrA\",4,5,\"+\"\n";
//    expect_error("to be 'seqnames'", buffer, 1, false, std::unordered_set<std::string>{ "chrA", "chrB" }); 
//
//    buffer = "\"seqnames\",\"foo\",\"end\",\"strand\"\n";
//    buffer += "\"chrA\",4,5,\"+\"\n";
//    expect_error("to be 'start'", buffer, 1, false, std::unordered_set<std::string>{ "chrA", "chrB" }); 
//
//    buffer = "\"seqnames\",\"start\",\"foo\",\"strand\"\n";
//    buffer += "\"chrA\",4,5,\"+\"\n";
//    expect_error("to be 'end'", buffer, 1, false, std::unordered_set<std::string>{ "chrA", "chrB" }); 
//
//    buffer = "\"seqnames\",\"start\",\"end\",\"foo\"\n";
//    buffer += "\"chrA\",4,5,\"+\"\n";
//    expect_error("to be 'strand'", buffer, 1, false, std::unordered_set<std::string>{ "chrA", "chrB" }); 
//}
