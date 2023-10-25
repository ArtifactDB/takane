#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "takane/genomic_ranges.hpp"

#include <fstream>
#include <string>

static void validate(const std::string& buffer, size_t num_ranges, bool has_names, const std::unordered_set<std::string>& all_seqnames) {
    std::string path = "TEST-genomic_ranges.csv";
    {
        std::ofstream ohandle(path);
        ohandle << buffer;
    }
    takane::genomic_ranges::validate(path.c_str(), num_ranges, has_names, all_seqnames);
}

template<typename ... Args_>
static void expect_error(const std::string& msg, const std::string& buffer, Args_&& ... args) {
    EXPECT_ANY_THROW({
        try {
            validate(buffer, std::forward<Args_>(args)...);
        } catch (std::exception& e) {
            EXPECT_THAT(e.what(), ::testing::HasSubstr(msg));
            throw;
        }
    });
}

TEST(GenomicRanges, Names) {
    std::string buffer = "\"names\",\"seqnames\",\"start\",\"end\",\"strand\"\n";
    buffer += "\"foo\",\"chrA\",4,10,\"*\"\n";
    buffer += "\"bar\",\"chrB\",9,9,\"+\"\n";
    buffer += "\"whee\",\"chrC\",19,100,\"-\"\n";
    validate(buffer, 3, true, std::unordered_set<std::string>{ "chrA", "chrB", "chrC" });

    expect_error("number of fields", buffer, 3, false, std::unordered_set<std::string>{ "chrA", "chrB", "chrC" }); 
    expect_error("number of records", buffer, 10, true, std::unordered_set<std::string>{ "chrA", "chrB", "chrC" }); 

    buffer = "\"seqnames\",\"start\",\"end\",\"strand\"\n";
    buffer += "\"chrA\",4,10,\"*\"\n";
    buffer += "\"chrB\",9,9,\"+\"\n";
    buffer += "\"chrC\",19,100,\"-\"\n";
    validate(buffer, 3, false, std::unordered_set<std::string>{ "chrA", "chrB", "chrC" }); 

    buffer = "\"names\",\"seqnames\",\"start\",\"end\",\"strand\"\n";
    buffer += "NA,\"chrA\",4,10,\"*\"\n";
    buffer += "NA,\"chrB\",9,9,\"+\"\n";
    buffer += "NA,\"chrC\",19,100,\"-\"\n";
    expect_error("missing values", buffer, 3, true, std::unordered_set<std::string>{ "chrA", "chrB", "chrC" }); 
}

TEST(GenomicRanges, Seqnames) {
    std::string buffer = "\"seqnames\",\"start\",\"end\",\"strand\"\n";
    buffer += "\"chrA\",4,10,\"*\"\n";
    buffer += "\"chrB\",9,9,\"+\"\n";
    buffer += "\"chrC\",19,100,\"-\"\n";
    expect_error("unknown sequence name", buffer, 3, false, std::unordered_set<std::string>{ "chrA", "chrB" }); 

    buffer = "\"seqnames\",\"start\",\"end\",\"strand\"\n";
    buffer += "NA,4,10,\"*\"\n";
    expect_error("missing values", buffer, 1, false, std::unordered_set<std::string>{ "chrA" }); 
}

TEST(GenomicRanges, Start) {
    std::string buffer = "\"seqnames\",\"start\",\"end\",\"strand\"\n";
    buffer += "\"chrA\",4.5,10,\"*\"\n";
    expect_error("is not an integer", buffer, 1, false, std::unordered_set<std::string>{ "chrA", "chrB" }); 

    buffer = "\"seqnames\",\"start\",\"end\",\"strand\"\n";
    buffer += "\"chrA\",3000000000,10,\"*\"\n";
    expect_error("does not fit", buffer, 1, false, std::unordered_set<std::string>{ "chrA", "chrB" }); 

    buffer = "\"seqnames\",\"start\",\"end\",\"strand\"\n";
    buffer += "\"chrA\",NA,10,\"*\"\n";
    expect_error("missing value", buffer, 1, false, std::unordered_set<std::string>{ "chrA", "chrB" }); 
}

TEST(GenomicRanges, End) {
    std::string buffer = "\"seqnames\",\"start\",\"end\",\"strand\"\n";
    buffer += "\"chrA\",4,5.1,\"*\"\n";
    expect_error("is not an integer", buffer, 1, false, std::unordered_set<std::string>{ "chrA", "chrB" }); 

    buffer = "\"seqnames\",\"start\",\"end\",\"strand\"\n";
    buffer += "\"chrA\",10,3000000000,\"*\"\n";
    expect_error("does not fit", buffer, 1, false, std::unordered_set<std::string>{ "chrA", "chrB" }); 

    buffer = "\"seqnames\",\"start\",\"end\",\"strand\"\n";
    buffer += "\"chrA\",10,NA,\"*\"\n";
    expect_error("missing value", buffer, 1, false, std::unordered_set<std::string>{ "chrA", "chrB" }); 

    buffer = "\"seqnames\",\"start\",\"end\",\"strand\"\n";
    buffer += "\"chrB\",99,108,\"*\"\n";
    buffer += "\"chrA\",10,8,\"*\"\n";
    buffer += "\"chrB\",50,100,\"*\"\n";
    expect_error("greater than or equal to", buffer, 3, false, std::unordered_set<std::string>{ "chrA", "chrB" }); 

    buffer = "\"seqnames\",\"start\",\"end\",\"strand\"\n";
    buffer += "\"chrA\",10,9,\"*\"\n";
    validate(buffer, 1, false, std::unordered_set<std::string>{ "chrA", "chrB", "chrC" });
}

TEST(GenomicRanges, Strand) {
    std::string buffer = "\"seqnames\",\"start\",\"end\",\"strand\"\n";
    buffer += "\"chrA\",4,5,NA\n";
    expect_error("missing value", buffer, 1, false, std::unordered_set<std::string>{ "chrA", "chrB" }); 

    buffer = "\"seqnames\",\"start\",\"end\",\"strand\"\n";
    buffer += "\"chrA\",4,51,\"5\"\n";
    expect_error("invalid strand", buffer, 1, false, std::unordered_set<std::string>{ "chrA", "chrB" }); 
}

TEST(GenomicRanges, ColumnNames) {
    std::string buffer = "\"foo\",\"start\",\"end\",\"strand\"\n";
    buffer += "\"chrA\",4,5,\"+\"\n";
    expect_error("to be 'seqnames'", buffer, 1, false, std::unordered_set<std::string>{ "chrA", "chrB" }); 

    buffer = "\"seqnames\",\"foo\",\"end\",\"strand\"\n";
    buffer += "\"chrA\",4,5,\"+\"\n";
    expect_error("to be 'start'", buffer, 1, false, std::unordered_set<std::string>{ "chrA", "chrB" }); 

    buffer = "\"seqnames\",\"start\",\"foo\",\"strand\"\n";
    buffer += "\"chrA\",4,5,\"+\"\n";
    expect_error("to be 'end'", buffer, 1, false, std::unordered_set<std::string>{ "chrA", "chrB" }); 

    buffer = "\"seqnames\",\"start\",\"end\",\"foo\"\n";
    buffer += "\"chrA\",4,5,\"+\"\n";
    expect_error("to be 'strand'", buffer, 1, false, std::unordered_set<std::string>{ "chrA", "chrB" }); 
}
