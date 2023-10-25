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
    takane::genomic_ranges::validate(path.c_str(), num_ranges, has_names, all_seqnames, comservatory::ReadOptions());
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
    validate(buffer, 3, true, std::unordered_set<std::string>{ "foo", "bar", "whee" });

    expect_error("number of fields", buffer, 3, false, std::unordered_set<std::string>{ "foo", "bar", "whee" });

    buffer = "\"seqnames\",\"start\",\"end\",\"strand\"\n";
    buffer += "\"chrA\",4,10,\"*\"\n";
    buffer += "\"chrB\",9,9,\"+\"\n";
    buffer += "\"chrC\",19,100,\"-\"\n";
    validate(buffer, 3, false, std::unordered_set<std::string>{ "foo", "bar", "whee" });
}
