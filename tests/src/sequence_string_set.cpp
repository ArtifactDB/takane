#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "takane/takane.hpp"
#include "sequence_string_set.h"
#include "data_frame.h"
#include "simple_list.h"

#include <fstream>
#include <string>

struct SequenceStringSetTest : public ::testing::Test {
    SequenceStringSetTest() {
        dir = "TEST_sequence_string_set";
        name = "sequence_string_set";
    }

    std::filesystem::path dir;
    std::string name;

    template<typename ... Args_>
    void expect_error(const std::string& msg, Args_&& ... args) {
        EXPECT_ANY_THROW({
            try {
                takane::validate(dir, std::forward<Args_>(args)...);
            } catch (std::exception& e) {
                EXPECT_THAT(e.what(), ::testing::HasSubstr(msg));
                throw;
            }
        });
    }
};

TEST_F(SequenceStringSetTest, MetadataRetrieval) {
    initialize_directory(dir);
    {
        std::ofstream handle(dir / "OBJECT");
        handle << "{ \"type\": \"sequence_string_set\", \"sequence_string_set\": { \"version\": \"2.0\" } }";
    }
    expect_error("unsupported version");

    // Checking the length.
    {
        std::ofstream handle(dir / "OBJECT");
        handle << "{ \"type\": \"sequence_string_set\", \"sequence_string_set\": { \"version\": \"1.0\" } }";
    }
    expect_error("expected a 'sequence_string_set.length' property");

    {
        std::ofstream handle(dir / "OBJECT");
        handle << "{ \"type\": \"sequence_string_set\", \"sequence_string_set\": { \"version\": \"1.0\", \"length\": true } }";
    }
    expect_error("JSON number");

    {
        std::ofstream handle(dir / "OBJECT");
        handle << "{ \"type\": \"sequence_string_set\", \"sequence_string_set\": { \"version\": \"1.0\", \"length\": 1.5 } }";
    }
    expect_error("non-negative integer");

    {
        std::ofstream handle(dir / "OBJECT");
        handle << "{ \"type\": \"sequence_string_set\", \"sequence_string_set\": { \"version\": \"1.0\", \"length\": -10 } }";
    }
    expect_error("non-negative integer");

    // Checking the sequence type.
    {
        std::ofstream handle(dir / "OBJECT");
        handle << "{ \"type\": \"sequence_string_set\", \"sequence_string_set\": { \"version\": \"1.0\", \"length\": 10 } }";
    }
    expect_error("failed to extract 'sequence_string_set.sequence_type'");

    {
        std::ofstream handle(dir / "OBJECT");
        handle << "{ \"type\": \"sequence_string_set\", \"sequence_string_set\": { \"version\": \"1.0\", \"length\": 10, \"sequence_type\": \"foobar\" } }";
    }
    expect_error("invalid string 'foobar'");

    // Checking the quality type.
    {
        std::ofstream handle(dir / "OBJECT");
        handle << "{ \"type\": \"sequence_string_set\", \"sequence_string_set\": { \"version\": \"1.0\", \"length\": 10, \"sequence_type\": \"RNA\", \"quality_type\": true } }";
    }
    expect_error("should be a JSON string");

    {
        std::ofstream handle(dir / "OBJECT");
        handle << "{ \"type\": \"sequence_string_set\", \"sequence_string_set\": { \"version\": \"1.0\", \"length\": 10, \"sequence_type\": \"RNA\", \"quality_type\": \"blah\" } }";
    }
    expect_error("invalid string 'blah'");

    {
        std::ofstream handle(dir / "OBJECT");
        handle << "{ \"type\": \"sequence_string_set\", \"sequence_string_set\": { \"version\": \"1.0\", \"length\": 10, \"sequence_type\": \"RNA\", \"quality_type\": \"phred\" } }";
    }
    expect_error("expected a 'sequence_string_set.quality_offset'");

    {
        std::ofstream handle(dir / "OBJECT");
        handle << "{ \"type\": \"sequence_string_set\", \"sequence_string_set\": { \"version\": \"1.0\", \"length\": 10, \"sequence_type\": \"RNA\", ";
        handle << "\"quality_type\": \"phred\", \"quality_offset\": true } }";
    }
    expect_error("JSON number");

    {
        std::ofstream handle(dir / "OBJECT");
        handle << "{ \"type\": \"sequence_string_set\", \"sequence_string_set\": { \"version\": \"1.0\", \"length\": 10, \"sequence_type\": \"RNA\", ";
        handle << "\"quality_type\": \"phred\", \"quality_offset\": 10 } }";
    }
    expect_error("33 or 64");
}


TEST_F(SequenceStringSetTest, FastaParsing) {
    sequence_string_set::Options options;
    sequence_string_set::mock(dir, 200, options);
    takane::validate(dir); // OKAY.
    EXPECT_EQ(takane::height(dir), 200);

    // Non-parallelized.
    {
        takane::Options inopt;
        inopt.parallel_reads = false;
        takane::validate(dir, inopt); 
    }

    // Name checks.
    auto spath = dir / "sequences.fasta.gz";
    {
        {
            byteme::GzipFileWriter writer(spath.c_str());
            writer.write("foobar\n");
        }
        expect_error("sequence name should start with '>'");

        {
            byteme::GzipFileWriter writer(spath.c_str());
            writer.write(">foobar\n");
        }
        expect_error("sequence name should be a non-negative integer");

        {
            byteme::GzipFileWriter writer(spath.c_str());
            writer.write(">\n");
        }
        expect_error("sequence name should be its index");

        {
            byteme::GzipFileWriter writer(spath.c_str());
            for (size_t i = 0; i < 200; ++i) {
                sequence_string_set::dump_fasta(writer, i % 10, "ACGT");
            }
        }
        expect_error("sequence name should be its index");
    }

    // Structural checks for correct parsing.
    {
        sequence_string_set::mock(dir, 2, options);

        // Zero length sequences are okay.
        {
            byteme::GzipFileWriter writer(spath.c_str());
            writer.write(">0\nACGT\n");
            writer.write(">1\n\n");
        }
        takane::validate(dir);

        // But they must be newline-terminated.
        {
            byteme::GzipFileWriter writer(spath.c_str());
            writer.write(">0\nACGT\n");
            writer.write(">1\n");
        }
        expect_error("premature end");
    }

    // Checking the count.
    sequence_string_set::mock(dir, 5, options);
    {
        byteme::GzipFileWriter writer(spath.c_str());
        sequence_string_set::dump_fasta(writer, 0, "AAAAAACCCCGGGGTTTT");
    }
    expect_error("observed number of sequences");

    // Simulating sequences with annoying newlines everywhere.
    sequence_string_set::mock(dir, 5, options);
    {
        byteme::GzipFileWriter writer(spath.c_str());
        sequence_string_set::dump_fasta(writer, 0, "AAAAAACCCCGGGGTTTT");
        sequence_string_set::dump_fasta(writer, 1, "\nAAAAAACCCCGGGGTTTT");
        sequence_string_set::dump_fasta(writer, 2, "AAAAAACCCCGGGGTTTT\n");
        sequence_string_set::dump_fasta(writer, 3, "AAAAAACCCC\nGGGGTTTT");
        sequence_string_set::dump_fasta(writer, 4, "AAAAAA\nCCCC\nGGGG\nTTTT");
    }
    takane::validate(dir); // OKAY.
}

TEST_F(SequenceStringSetTest, FastqParsing) {
    sequence_string_set::Options options;
    options.quality_type = sequence_string_set::QualityType::PHRED33;
    sequence_string_set::mock(dir, 200, options);
    takane::validate(dir); // OKAY.

    // Non-parallelized.
    {
        takane::Options inopt;
        inopt.parallel_reads = false;
        takane::validate(dir, inopt); 
    }

    // Name checks.
    auto spath = dir / "sequences.fastq.gz";
    {
        {
            byteme::GzipFileWriter writer(spath.c_str());
            writer.write("foobar\n");
        }
        expect_error("sequence name should start with '@'");

        {
            byteme::GzipFileWriter writer(spath.c_str());
            writer.write("@foobar\n");
        }
        expect_error("sequence name should be a non-negative integer");

        {
            byteme::GzipFileWriter writer(spath.c_str());
            writer.write("@\n");
        }
        expect_error("sequence name should be its index");

        {
            byteme::GzipFileWriter writer(spath.c_str());
            for (size_t i = 0; i < 200; ++i) {
                sequence_string_set::dump_fastq(writer, i % 10, "ACGT", "FECD");
            }
        }
        expect_error("sequence name should be its index");
    }

    // Structural checks for correct parsing.
    {
        sequence_string_set::mock(dir, 2, options);

        // Ignores gunk on the + line.
        {
            byteme::GzipFileWriter writer(spath.c_str());
            writer.write("@0\nACGT\n+123123123\nFECD\n");
            writer.write("@1\nACGT\n+ foo bar\nFECD\n");
        }
        takane::validate(dir);

        // Works when you have '@' in the quality scores.
        {
            byteme::GzipFileWriter writer(spath.c_str());
            writer.write("@0\nACGT\n+\n@@@@\n");
            writer.write("@1\nACGT\n+\n++++\n");
        }
        takane::validate(dir);

        // Fails with a mismatch in the lengths.
        {
            byteme::GzipFileWriter writer(spath.c_str());
            writer.write("@0\nACGT\n+\n@@@\n");
            writer.write("@1\nACGT\n+\n++++\n");
        }
        expect_error("unequal lengths");

        {
            byteme::GzipFileWriter writer(spath.c_str());
            writer.write("@0\nACGTACGTACGT\n+\n@@@@\n@@@@\n@@@@\n");
            writer.write("@1\nACGT\n+\n++++\n");
        }
        takane::validate(dir); // OK!

        // More complicated mismatch in the lengths.
        {
            byteme::GzipFileWriter writer(spath.c_str());
            writer.write("@0\nACGTACGTACGT\n+\n@@@@\n@@@@\n@\n@@@@\n");
            writer.write("@1\nACGT\n+\n++++\n");
        }
        expect_error("unequal lengths");

        // Zero length sequences are okay.
        {
            byteme::GzipFileWriter writer(spath.c_str());
            writer.write("@0\n\n+\n\n");
            writer.write("@1\n\n+\n\n");
        }
        takane::validate(dir);

        // But they must be newline-terminated.
        {
            byteme::GzipFileWriter writer(spath.c_str());
            writer.write("@0\n\n+\n\n");
            writer.write("@1\n\n+\n");
        }
        expect_error("premature end");
    }

    // Checking the count.
    sequence_string_set::mock(dir, 5, options);
    {
        byteme::GzipFileWriter writer(spath.c_str());
        sequence_string_set::dump_fastq(writer, 0, "AAAACCCCGGGGTTTT", "~~~~!!!!AAAAFFFF");
    }
    expect_error("observed number of sequences");

    // Simulating sequences with annoying newlines everywhere.
    sequence_string_set::mock(dir, 10, options);
    {
        byteme::GzipFileWriter writer(spath.c_str());
        sequence_string_set::dump_fastq(writer, 0, "AAAACCCCGGGGTTTT", "FFFFEEEEDDDDCCCC");
        sequence_string_set::dump_fastq(writer, 1, "\nAAAACCCCGGGGTTTT", "FFFFEEEEDDDDCCCC");
        sequence_string_set::dump_fastq(writer, 2, "AAAACCCCGGGGTTTT\n", "FFFFEEEEDDDDCCCC");
        sequence_string_set::dump_fastq(writer, 3, "AAAACCCC\nGGGGTTTT", "FFFFEEEEDDDDCCCC");
        sequence_string_set::dump_fastq(writer, 4, "AAAA\nCCCC\nGGGG\nTTTT", "FFFFEEEEDDDDCCCC");
        sequence_string_set::dump_fastq(writer, 5, "AAAACCCCGGGGTTTT", "\nFFFFEEEEDDDDCCCC");
        sequence_string_set::dump_fastq(writer, 6, "AAAACCCCGGGGTTTT", "FFFFEEEEDDDDCCCC\n");
        sequence_string_set::dump_fastq(writer, 7, "AAAACCCCGGGGTTTT", "FFFFEEEEDDDD\nCCCC");
        sequence_string_set::dump_fastq(writer, 8, "AAAACCCCGGGGTTTT", "FFFFEEEE\nDDDDCCCC");
        sequence_string_set::dump_fastq(writer, 9, "AAAACCCCGGGGTTTT", "FFFF\nEEEE\nDDDD\nCCCC");
    }
    takane::validate(dir); // OKAY.
}

TEST_F(SequenceStringSetTest, SequenceTypes) {
    sequence_string_set::Options options;
    auto spath = dir / "sequences.fasta.gz";

    // DNA.
    {
        options.sequence_type = sequence_string_set::SequenceType::DNA;
        sequence_string_set::mock(dir, 3, options);
        takane::validate(dir);

        {
            byteme::GzipFileWriter writer(spath.c_str());
            writer.write(">0\nacgt\n");
            writer.write(">1\na.c-g.tacgryswkmbdhvn.-\n");
            writer.write(">2\nTACGRYSWKMBDHVN.-\n");
        }
        takane::validate(dir);

        {
            byteme::GzipFileWriter writer(spath.c_str());
            writer.write(">0\nacgt\n");
            writer.write(">1\nuuuu\n");
            writer.write(">2\nACGT\n");
        }
        expect_error("forbidden character 'u'");
    }

    // FASTQ, for DNA.
    {
        options.quality_type = sequence_string_set::QualityType::PHRED33;
        sequence_string_set::mock(dir, 1, options);

        auto spath = dir / "sequences.fastq.gz";
        {
            byteme::GzipFileWriter writer(spath.c_str());
            writer.write("@0\nacgu\n+\n;;;;\n");
        }
        expect_error("forbidden character 'u'");

        options.quality_type = sequence_string_set::QualityType::NONE;
    }

    // RNA.
    {
        options.sequence_type = sequence_string_set::SequenceType::RNA;
        sequence_string_set::mock(dir, 3, options);
        takane::validate(dir);

        {
            byteme::GzipFileWriter writer(spath.c_str());
            writer.write(">0\nacgU\n");
            writer.write(">1\na.c-g.uacgryswkmbdhvn.-\n");
            writer.write(">2\nUACGRYSWKMBDHVN.-\n");
        }
        takane::validate(dir);

        {
            byteme::GzipFileWriter writer(spath.c_str());
            writer.write(">0\nacgu\n");
            writer.write(">1\nTTTT\n");
            writer.write(">2\nACGU\n");
        }
        expect_error("forbidden character 'T'");
    }

    // Protein.
    {
        options.sequence_type = sequence_string_set::SequenceType::AA;
        sequence_string_set::mock(dir, 3, options);
        takane::validate(dir);

        {
            byteme::GzipFileWriter writer(spath.c_str());
            writer.write(">0\nacgt\n");
            writer.write(">1\nACD.EFGHIKLMNP-QRSTVWY\n");
            writer.write(">2\nacd.efghiklmnp-qrstvwy\n");
        }
        takane::validate(dir);

        {
            byteme::GzipFileWriter writer(spath.c_str());
            writer.write(">0\nacgt\n");
            writer.write(">1\nxxxx\n");
            writer.write(">2\nACGU\n");
        }
        expect_error("forbidden character 'x'");
    }

    // Custom.
    {
        options.sequence_type = sequence_string_set::SequenceType::CUSTOM;
        sequence_string_set::mock(dir, 3, options);
        takane::validate(dir);

        {
            byteme::GzipFileWriter writer(spath.c_str());
            writer.write(">0\nxxxx\n");
            writer.write(">1\n!;;;*8acac~\n");
            writer.write(">2\nacd.efghiklmnp-qrstvwy\n");
        }
        takane::validate(dir);
    }
}

TEST_F(SequenceStringSetTest, QualityType) {
    sequence_string_set::Options options;
    auto spath = dir / "sequences.fastq.gz";

    // Phred+33.
    {
        options.quality_type = sequence_string_set::QualityType::PHRED33;
        sequence_string_set::mock(dir, 3, options);
        takane::validate(dir);

        {
            byteme::GzipFileWriter writer(spath.c_str());
            writer.write("@0\nacgt\n+\n!!!!\n");
            writer.write("@1\nacgt\n+\n!!!!\n");
            writer.write("@2\nacgt\n+\n!!!!\n");
        }
        takane::validate(dir);

        {
            byteme::GzipFileWriter writer(spath.c_str());
            writer.write("@0\nacgt\n+\n\1\1\1\1\n");
            writer.write("@1\nacgt\n+\n!!!!\n");
            writer.write("@2\nacgt\n+\n!!!!\n");
        }
        expect_error("out-of-range quality score");
    }

    // Phred+64.
    {
        options.quality_type = sequence_string_set::QualityType::ILLUMINA64;
        sequence_string_set::mock(dir, 3, options);
        takane::validate(dir);

        {
            byteme::GzipFileWriter writer(spath.c_str());
            writer.write("@0\nacgt\n+\n@@@@\n");
            writer.write("@1\nacgt\n+\n@@@@\n");
            writer.write("@2\nacgt\n+\n@@@@\n");
        }
        takane::validate(dir);

        {
            byteme::GzipFileWriter writer(spath.c_str());
            writer.write("@0\nacgt\n+\n????\n");
            writer.write("@1\nacgt\n+\n@@@@\n");
            writer.write("@2\nacgt\n+\n@@@@\n");
        }
        expect_error("out-of-range quality score");
    }

    // Solexa+64.
    {
        options.quality_type = sequence_string_set::QualityType::SOLEXA;
        sequence_string_set::mock(dir, 3, options);
        takane::validate(dir);

        {
            byteme::GzipFileWriter writer(spath.c_str());
            writer.write("@0\nacgt\n+\n;;;;\n");
            writer.write("@1\nacgt\n+\n@@@@\n");
            writer.write("@2\nacgt\n+\n@@@@\n");
        }
        takane::validate(dir);

        {
            byteme::GzipFileWriter writer(spath.c_str());
            writer.write("@0\nacgt\n+\n::::\n");
            writer.write("@1\nacgt\n+\n@@@@\n");
            writer.write("@2\nacgt\n+\n@@@@\n");
        }
        expect_error("out-of-range quality score");
    }

    // None.
    {
        options.quality_type = sequence_string_set::QualityType::NONE;
        sequence_string_set::mock(dir, 10, options);
        {
            std::ofstream handle(dir / "OBJECT");
            handle << "{ \"type\": \"sequence_string_set\", \"sequence_string_set\": { \"version\": \"1.0\", \"length\": 10, \"sequence_type\": \"RNA\", \"quality_type\": \"none\" } }";
        }
        takane::validate(dir);
    }
}

TEST_F(SequenceStringSetTest, Metadata) {
    sequence_string_set::Options options;
    sequence_string_set::mock(dir, 20, options);

    auto odir = dir / "other_annotations";
    auto rdir = dir / "sequence_annotations";

    initialize_directory_simple(rdir, "simple_list", "1.0");
    expect_error("'DATA_FRAME'"); 

    data_frame::mock(rdir, 20, {});
    initialize_directory_simple(odir, "data_frame", "1.0");
    expect_error("'SIMPLE_LIST'");

    simple_list::mock(odir);
    takane::validate(dir);
}
