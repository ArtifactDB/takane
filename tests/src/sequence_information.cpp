#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "takane/sequence_information.hpp"
#include "utils.h"

#include <fstream>
#include <string>

static void validate(const std::string& buffer, size_t num_sequences) {
    std::string path = "TEST-seqinfo.csv";
    {
        std::ofstream ohandle(path);
        ohandle << buffer;
    }
    takane::sequence_information::Parameters params;
    params.num_sequences = num_sequences;
    takane::sequence_information::validate(path.c_str(), params);
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

TEST(SequenceInformation, Basic) {
    std::string buffer = "\"seqnames\",\"seqlengths\",\"isCircular\",\"genome\"\n";
    buffer += "\"chrA\",4,TRUE,\"hg19\"\n";
    buffer += "\"chrB\",9,FALSE,\"mm10\"\n";
    buffer += "\"chrC\",19,TRUE,\"hg38\"\n";
    validate(buffer, 3);
    expect_error("number of records", buffer, 10);

    buffer = "\"whee\",\"seqlengths\",\"isCircular\",\"genome\"\n";
    buffer += "\"chrA\",4,TRUE,\"hg19\"\n";
    buffer += "\"chrB\",9,FALSE,\"mm10\"\n";
    buffer += "\"chrC\",19,TRUE,\"hg38\"\n";
    expect_error("'seqnames'", buffer, 10);

    buffer = "\"seqlengths\",\"isCircular\",\"genome\"\n";
    buffer += "4,TRUE,\"hg19\"\n";
    buffer += "9,FALSE,\"mm10\"\n";
    buffer += "19,TRUE,\"hg38\"\n";
    expect_error("number of header names", buffer, 10);
}

TEST(SequenceInformation, Seqnames) {
    std::string buffer = "\"seqnames\",\"seqlengths\",\"isCircular\",\"genome\"\n";
    buffer += "\"chrA\",4,TRUE,\"hg19\"\n";
    buffer += "\"chrB\",9,FALSE,\"mm10\"\n";
    buffer += "\"chrA\",19,TRUE,\"hg38\"\n";
    expect_error("duplicated value", buffer, 3);

    buffer = "\"seqnames\",\"seqlengths\",\"isCircular\",\"genome\"\n";
    buffer += "NA,4,TRUE,\"hg19\"\n";
    expect_error("missing value", buffer, 1);

    buffer = "\"seqnames\",\"seqlengths\",\"isCircular\",\"genome\"\n";
    buffer += "\"chrA\",4,TRUE,\"hg19\"\n";
    buffer += "\"chrB\",9,FALSE,\"mm10\"\n";
    buffer += "\"chrX\",99,FALSE,\"mm10\"\n";
    {
        takane::sequence_information::Parameters opt;
        opt.num_sequences = 3;
        byteme::RawBufferReader input(reinterpret_cast<const unsigned char*>(buffer.c_str()), buffer.size());

        FilledFieldCreator filled; 
        auto output = takane::sequence_information::validate(input, opt, &filled);
        std::vector<std::string> expected{ "chrA", "chrB", "chrX" };
        EXPECT_EQ(static_cast<comservatory::FilledStringField*>(output.fields[0].get())->values, expected);
    }
}

TEST(SequenceInformation, Seqlengths) {
    std::string buffer = "\"seqnames\",\"seqlengths\",\"isCircular\",\"genome\"\n";
    buffer += "\"chrA\",NA,TRUE,\"hg19\"\n";
    buffer += "\"chrB\",NA,FALSE,\"mm10\"\n";
    validate(buffer, 2); // missing is allowed.

    buffer = "\"seqnames\",\"seqlengths\",\"isCircular\",\"genome\"\n";
    buffer += "\"chrA\",1.2,TRUE,\"hg19\"\n";
    buffer += "\"chrB\",20.4,FALSE,\"mm10\"\n";
    buffer += "\"chrC\",22.1,TRUE,\"hg38\"\n";
    expect_error("integer", buffer, 3);

    buffer = "\"seqnames\",\"seqlengths\",\"isCircular\",\"genome\"\n";
    buffer += "\"chrA\",-1,TRUE,\"hg19\"\n";
    buffer += "\"chrB\",24,FALSE,\"mm10\"\n";
    buffer += "\"chrC\",21,TRUE,\"hg38\"\n";
    expect_error("negative", buffer, 3);

    buffer = "\"seqnames\",\"seqlengths\",\"isCircular\",\"genome\"\n";
    buffer += "\"chrA\",1,TRUE,\"hg19\"\n";
    buffer += "\"chrB\",24,FALSE,\"mm10\"\n";
    buffer += "\"chrC\",3100000000,TRUE,\"hg38\"\n";
    expect_error("32-bit", buffer, 3);
}
