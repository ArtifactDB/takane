#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "takane/vcf_experiment.hpp"
#include "utils.h"
#include "sequence_string_set.h"
#include "data_frame.h"
#include "simple_list.h"

#include <fstream>
#include <string>

struct VcfExperimentTest : public ::testing::Test {
    VcfExperimentTest() {
        dir = "TEST_sequence_string_set";
        name = "sequence_string_set";
    }

    std::filesystem::path dir;
    std::string name;

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

TEST_F(VcfExperimentTest, MetadataRetrieval) {
    initialize_directory(dir);
    {
        std::ofstream handle(dir / "OBJECT");
        handle << "{ \"type\": \"vcf_experiment\", \"vcf_experiment\": { \"version\": \"2.0\" } }";
    }
    expect_error("unsupported version");

    // Checking the length.
    {
        std::ofstream handle(dir / "OBJECT");
        handle << "{ \"type\": \"vcf_experiment\", \"vcf_experiment\": { \"version\": \"1.0\" } }";
    }
    expect_error("expected a 'sequence_string_set.dimensions' property");

//    {
//        std::ofstream handle(dir / "OBJECT");
//        handle << "{ \"type\": \"sequence_string_set\", \"sequence_string_set\": { \"version\": \"1.0\", \"length\": true } }";
//    }
//    expect_error("JSON number");
//
//    {
//        std::ofstream handle(dir / "OBJECT");
//        handle << "{ \"type\": \"sequence_string_set\", \"sequence_string_set\": { \"version\": \"1.0\", \"length\": 1.5 } }";
//    }
//    expect_error("non-negative integer");
//
//    {
//        std::ofstream handle(dir / "OBJECT");
//        handle << "{ \"type\": \"sequence_string_set\", \"sequence_string_set\": { \"version\": \"1.0\", \"length\": -10 } }";
//    }
//    expect_error("non-negative integer");
//
//    // Checking the sequence type.
//    {
//        std::ofstream handle(dir / "OBJECT");
//        handle << "{ \"type\": \"sequence_string_set\", \"sequence_string_set\": { \"version\": \"1.0\", \"length\": 10 } }";
//    }
//    expect_error("failed to extract 'sequence_string_set.sequence_type'");
//
//    {
//        std::ofstream handle(dir / "OBJECT");
//        handle << "{ \"type\": \"sequence_string_set\", \"sequence_string_set\": { \"version\": \"1.0\", \"length\": 10, \"sequence_type\": \"foobar\" } }";
//    }
//    expect_error("invalid string 'foobar'");
//
//    // Checking the quality type.
//    {
//        std::ofstream handle(dir / "OBJECT");
//        handle << "{ \"type\": \"sequence_string_set\", \"sequence_string_set\": { \"version\": \"1.0\", \"length\": 10, \"sequence_type\": \"RNA\", \"quality_type\": true } }";
//    }
//    expect_error("should be a JSON string");
//
//    {
//        std::ofstream handle(dir / "OBJECT");
//        handle << "{ \"type\": \"sequence_string_set\", \"sequence_string_set\": { \"version\": \"1.0\", \"length\": 10, \"sequence_type\": \"RNA\", \"quality_type\": \"blah\" } }";
//    }
//    expect_error("invalid string 'blah'");
//
//    {
//        std::ofstream handle(dir / "OBJECT");
//        handle << "{ \"type\": \"sequence_string_set\", \"sequence_string_set\": { \"version\": \"1.0\", \"length\": 10, \"sequence_type\": \"RNA\", \"quality_type\": \"phred\" } }";
//    }
//    expect_error("expected a 'sequence_string_set.quality_offset'");
//
//    {
//        std::ofstream handle(dir / "OBJECT");
//        handle << "{ \"type\": \"sequence_string_set\", \"sequence_string_set\": { \"version\": \"1.0\", \"length\": 10, \"sequence_type\": \"RNA\", ";
//        handle << "\"quality_type\": \"phred\", \"quality_offset\": true } }";
//    }
//    expect_error("JSON number");
//
//    {
//        std::ofstream handle(dir / "OBJECT");
//        handle << "{ \"type\": \"sequence_string_set\", \"sequence_string_set\": { \"version\": \"1.0\", \"length\": 10, \"sequence_type\": \"RNA\", ";
//        handle << "\"quality_type\": \"phred\", \"quality_offset\": 10 } }";
//    }
//    expect_error("33 or 64");
}

//TEST_F(VcfExperimentTest, FastaParsing) {
//    sequence_string_set::Options options;
//    sequence_string_set::mock(dir, 200, options);
//    test_validate(dir); // OKAY.
//    EXPECT_EQ(test_height(dir), 200);
//
//    // Non-parallelized.
//    {
//        takane::Options inopt;
//        inopt.parallel_reads = false;
//        auto meta = takane::read_object_metadata(dir);
//        takane::sequence_string_set::validate(dir, meta, inopt); 
//    }
//
//    // Name checks.
//    auto spath = dir / "sequences.fasta.gz";
//    {
//        {
//            byteme::GzipFileWriter writer(spath.c_str());
//            writer.write("foobar\n");
//        }
//        expect_error("sequence name should start with '>'");
//
//        {
//            byteme::GzipFileWriter writer(spath.c_str());
//            writer.write(">foobar\n");
//        }
//        expect_error("sequence name should be a non-negative integer");
//
//        {
//            byteme::GzipFileWriter writer(spath.c_str());
//            writer.write(">\n");
//        }
//        expect_error("sequence name should be its index");
//
//        {
//            byteme::GzipFileWriter writer(spath.c_str());
//            for (size_t i = 0; i < 200; ++i) {
//                sequence_string_set::dump_fasta(writer, i % 10, "ACGT");
//            }
//        }
//        expect_error("sequence name should be its index");
//    }
//
//    // Structural checks for correct parsing.
//    {
//        sequence_string_set::mock(dir, 2, options);
//
//        // Zero length sequences are okay.
//        {
//            byteme::GzipFileWriter writer(spath.c_str());
//            writer.write(">0\nACGT\n");
//            writer.write(">1\n\n");
//        }
//        test_validate(dir);
//
//        // But they must be newline-terminated.
//        {
//            byteme::GzipFileWriter writer(spath.c_str());
//            writer.write(">0\nACGT\n");
//            writer.write(">1\n");
//        }
//        expect_error("premature end");
//    }
//
//    // Checking the count.
//    sequence_string_set::mock(dir, 5, options);
//    {
//        byteme::GzipFileWriter writer(spath.c_str());
//        sequence_string_set::dump_fasta(writer, 0, "AAAAAACCCCGGGGTTTT");
//    }
//    expect_error("observed number of sequences");
//
//    // Simulating sequences with annoying newlines everywhere.
//    sequence_string_set::mock(dir, 5, options);
//    {
//        byteme::GzipFileWriter writer(spath.c_str());
//        sequence_string_set::dump_fasta(writer, 0, "AAAAAACCCCGGGGTTTT");
//        sequence_string_set::dump_fasta(writer, 1, "\nAAAAAACCCCGGGGTTTT");
//        sequence_string_set::dump_fasta(writer, 2, "AAAAAACCCCGGGGTTTT\n");
//        sequence_string_set::dump_fasta(writer, 3, "AAAAAACCCC\nGGGGTTTT");
//        sequence_string_set::dump_fasta(writer, 4, "AAAAAA\nCCCC\nGGGG\nTTTT");
//    }
//    test_validate(dir); // OKAY.
//}
