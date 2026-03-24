#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "takane/sequence_string_set.hpp"
#include "utils.h"
#include "sequence_string_set.h"
#include "data_frame.h"
#include "simple_list.h"

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
                test_validate(dir, std::forward<Args_>(args)...);
            } catch (std::exception& e) {
                EXPECT_THAT(e.what(), ::testing::HasSubstr(msg));
                throw;
            }
        });
    }
};

TEST_F(SequenceStringSetTest, MetadataRetrieval) {
    initialize_directory(dir);
    auto objpath = (dir / "OBJECT").string();

    quick_text_write(objpath,
        "{ \"type\": \"sequence_string_set\", \"sequence_string_set\": { \"version\": \"2.0\" } }"
    );
    expect_error("unsupported version");

    // Checking the length.
    quick_text_write(objpath,
        "{ \"type\": \"sequence_string_set\", \"sequence_string_set\": { \"version\": \"1.0\" } }"
    );
    expect_error("expected a 'sequence_string_set.length' property");

    quick_text_write(objpath,
        "{ \"type\": \"sequence_string_set\", \"sequence_string_set\": { \"version\": \"1.0\", \"length\": true } }"
    );
    expect_error("JSON number");

    quick_text_write(objpath,
        "{ \"type\": \"sequence_string_set\", \"sequence_string_set\": { \"version\": \"1.0\", \"length\": 1.5 } }"
    );
    expect_error("non-negative integer");

    quick_text_write(objpath,
        "{ \"type\": \"sequence_string_set\", \"sequence_string_set\": { \"version\": \"1.0\", \"length\": -10 } }"
    );
    expect_error("non-negative integer");

    // Checking the sequence type.
    quick_text_write(objpath,
        "{ \"type\": \"sequence_string_set\", \"sequence_string_set\": { \"version\": \"1.0\", \"length\": 10 } }"
    );
    expect_error("failed to extract 'sequence_string_set.sequence_type'");

    quick_text_write(objpath,
        "{ \"type\": \"sequence_string_set\", \"sequence_string_set\": { \"version\": \"1.0\", \"length\": 10, \"sequence_type\": \"foobar\" } }"
    );
    expect_error("invalid string 'foobar'");

    // Checking the quality type.
    quick_text_write(objpath,
        "{ \"type\": \"sequence_string_set\", \"sequence_string_set\": { \"version\": \"1.0\", \"length\": 10, \"sequence_type\": \"RNA\", \"quality_type\": true } }"
    );
    expect_error("should be a JSON string");

    quick_text_write(objpath,
        "{ \"type\": \"sequence_string_set\", \"sequence_string_set\": { \"version\": \"1.0\", \"length\": 10, \"sequence_type\": \"RNA\", \"quality_type\": \"blah\" } }"
    );
    expect_error("invalid string 'blah'");

    quick_text_write(objpath,
        "{ \"type\": \"sequence_string_set\", \"sequence_string_set\": { \"version\": \"1.0\", \"length\": 10, \"sequence_type\": \"RNA\", \"quality_type\": \"phred\" } }"
    );
    expect_error("expected a 'sequence_string_set.quality_offset'");

    quick_text_write(objpath,
        "{ \"type\": \"sequence_string_set\", \"sequence_string_set\": { \"version\": \"1.0\", \"length\": 10, \"sequence_type\": \"RNA\", "
        "\"quality_type\": \"phred\", \"quality_offset\": true } }"
    );
    expect_error("JSON number");

    quick_text_write(objpath,
        "{ \"type\": \"sequence_string_set\", \"sequence_string_set\": { \"version\": \"1.0\", \"length\": 10, \"sequence_type\": \"RNA\", "
        "\"quality_type\": \"phred\", \"quality_offset\": 10 } }"
    );
    expect_error("33 or 64");
}

TEST_F(SequenceStringSetTest, FastaParsing) {
    sequence_string_set::Options options;
    sequence_string_set::mock(dir, 200, options);
    test_validate(dir); // OKAY.
    EXPECT_EQ(test_height(dir), 200);

    // Non-parallelized.
    {
        takane::Options inopt;
        inopt.parallel_reads = false;
        auto meta = takane::read_object_metadata(dir);
        takane::sequence_string_set::validate(dir, meta, inopt); 
    }

    // Name checks.
    auto spath = dir / "sequences.fasta.gz";
    {
        quick_gzip_write(spath.string(), "foobar\n");
        expect_error("sequence name should start with '>'");

        quick_gzip_write(spath.string(), ">foobar\n");
        expect_error("sequence name should be a non-negative integer");

        quick_gzip_write(spath.string(), ">\n");
        expect_error("sequence name should be its index");

        {
            byteme::GzipFileWriter writer(spath.c_str(), {});
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
        quick_gzip_write(spath.string(),
            ">0\nACGT\n"
            ">1\n\n"
        );
        test_validate(dir);

        // But they must be newline-terminated.
        quick_gzip_write(spath.string(),
            ">0\nACGT\n"
            ">1\n"
        );
        expect_error("premature end");
    }

    // Checking the count.
    sequence_string_set::mock(dir, 5, options);
    {
        byteme::GzipFileWriter writer(spath.c_str(), {});
        sequence_string_set::dump_fasta(writer, 0, "AAAAAACCCCGGGGTTTT");
    }
    expect_error("observed number of sequences");

    // Simulating sequences with annoying newlines everywhere.
    sequence_string_set::mock(dir, 5, options);
    {
        byteme::GzipFileWriter writer(spath.c_str(), {});
        sequence_string_set::dump_fasta(writer, 0, "AAAAAACCCCGGGGTTTT");
        sequence_string_set::dump_fasta(writer, 1, "\nAAAAAACCCCGGGGTTTT");
        sequence_string_set::dump_fasta(writer, 2, "AAAAAACCCCGGGGTTTT\n");
        sequence_string_set::dump_fasta(writer, 3, "AAAAAACCCC\nGGGGTTTT");
        sequence_string_set::dump_fasta(writer, 4, "AAAAAA\nCCCC\nGGGG\nTTTT");
    }
    test_validate(dir); // OKAY.
}

TEST_F(SequenceStringSetTest, FastqParsing) {
    sequence_string_set::Options options;
    options.quality_type = sequence_string_set::QualityType::PHRED33;
    sequence_string_set::mock(dir, 200, options);
    test_validate(dir); // OKAY.

    // Non-parallelized.
    {
        takane::Options inopt;
        inopt.parallel_reads = false;
        auto meta = takane::read_object_metadata(dir);
        takane::validate(dir, meta, inopt); 
    }

    // Name checks.
    auto spath = dir / "sequences.fastq.gz";
    {
        quick_gzip_write(spath.string(), "foobar\n");
        expect_error("sequence name should start with '@'");

        quick_gzip_write(spath.string(), "@foobar\n");
        expect_error("sequence name should be a non-negative integer");

        quick_gzip_write(spath.string(), "@\n");
        expect_error("sequence name should be its index");

        {
            byteme::GzipFileWriter writer(spath.c_str(), {});
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
        quick_gzip_write(spath.string(),
            "@0\nACGT\n+123123123\nFECD\n"
            "@1\nACGT\n+ foo bar\nFECD\n"
        );
        test_validate(dir);

        // Works when you have '@' in the quality scores.
        quick_gzip_write(spath.string(),
            "@0\nACGT\n+\n@@@@\n"
            "@1\nACGT\n+\n++++\n"
        );
        test_validate(dir);

        // Fails with a mismatch in the lengths.
        quick_gzip_write(spath.string(),
            "@0\nACGT\n+\n@@@\n"
            "@1\nACGT\n+\n++++\n"
        );
        expect_error("unequal lengths");

        quick_gzip_write(spath.string(),
            "@0\nACGTACGTACGT\n+\n@@@@\n@@@@\n@@@@\n"
            "@1\nACGT\n+\n++++\n"
        );
        test_validate(dir); // OK!

        // More complicated mismatch in the lengths.
        quick_gzip_write(spath.string(),
            "@0\nACGTACGTACGT\n+\n@@@@\n@@@@\n@\n@@@@\n"
            "@1\nACGT\n+\n++++\n"
        );
        expect_error("unequal lengths");

        // Zero length sequences are okay.
        quick_gzip_write(spath.string(),
            "@0\n\n+\n\n"
            "@1\n\n+\n\n"
        );
        test_validate(dir);

        // But they must be newline-terminated.
        quick_gzip_write(spath.string(),
            "@0\n\n+\n\n"
            "@1\n\n+\n"
        );
        expect_error("premature end");
    }

    // Checking the count.
    sequence_string_set::mock(dir, 5, options);
    {
        byteme::GzipFileWriter writer(spath.c_str(), {});
        sequence_string_set::dump_fastq(writer, 0, "AAAACCCCGGGGTTTT", "~~~~!!!!AAAAFFFF");
    }
    expect_error("observed number of sequences");

    // Simulating sequences with annoying newlines everywhere.
    sequence_string_set::mock(dir, 10, options);
    {
        byteme::GzipFileWriter writer(spath.c_str(), {});
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
    test_validate(dir); // OKAY.
}

TEST_F(SequenceStringSetTest, SequenceTypes) {
    sequence_string_set::Options options;
    auto spath = (dir / "sequences.fasta.gz").string();

    // DNA.
    {
        options.sequence_type = sequence_string_set::SequenceType::DNA;
        sequence_string_set::mock(dir, 3, options);
        test_validate(dir);

        quick_gzip_write(spath,
            ">0\nacgt\n"
            ">1\na.c-g.tacgryswkmbdhvn.-\n"
            ">2\nTACGRYSWKMBDHVN.-\n"
        );
        test_validate(dir);

        quick_gzip_write(spath,
            ">0\nacgt\n"
            ">1\nuuuu\n"
            ">2\nACGT\n"
        );
        expect_error("forbidden character 'u'");
    }

    // FASTQ, for DNA.
    {
        options.quality_type = sequence_string_set::QualityType::PHRED33;
        sequence_string_set::mock(dir, 1, options);

        quick_gzip_write((dir / "sequences.fastq.gz").string(), "@0\nacgu\n+\n;;;;\n");
        expect_error("forbidden character 'u'");

        options.quality_type = sequence_string_set::QualityType::NONE;
    }

    // RNA.
    {
        options.sequence_type = sequence_string_set::SequenceType::RNA;
        sequence_string_set::mock(dir, 3, options);
        test_validate(dir);

        quick_gzip_write(spath,
            ">0\nacgU\n"
            ">1\na.c-g.uacgryswkmbdhvn.-\n"
            ">2\nUACGRYSWKMBDHVN.-\n"
        );
        test_validate(dir);

        quick_gzip_write(spath,
            ">0\nacgu\n"
            ">1\nTTTT\n"
            ">2\nACGU\n"
        );
        expect_error("forbidden character 'T'");
    }

    // Protein.
    {
        options.sequence_type = sequence_string_set::SequenceType::AA;
        sequence_string_set::mock(dir, 3, options);
        test_validate(dir);

        quick_gzip_write(spath,
            ">0\nacgt\n"
            ">1\nACD.EFGHIKLMNP-QRSTVWY\n"
            ">2\nacd.efghiklmnp-qrstvwy\n"
        );
        test_validate(dir);

        quick_gzip_write(spath,
            ">0\nacgt\n"
            ">1\nxxxx\n"
            ">2\nACGU\n"
        );
        expect_error("forbidden character 'x'");
    }

    // Custom.
    {
        options.sequence_type = sequence_string_set::SequenceType::CUSTOM;
        sequence_string_set::mock(dir, 3, options);
        test_validate(dir);

        quick_gzip_write(spath,
            ">0\nxxxx\n"
            ">1\n!;;;*8acac~\n"
            ">2\nacd.efghiklmnp-qrstvwy\n"
        );
        test_validate(dir);
    }
}

TEST_F(SequenceStringSetTest, QualityType) {
    sequence_string_set::Options options;
    auto spath = (dir / "sequences.fastq.gz").string();

    // Phred+33.
    {
        options.quality_type = sequence_string_set::QualityType::PHRED33;
        sequence_string_set::mock(dir, 3, options);
        test_validate(dir);

        quick_gzip_write(spath,
            "@0\nacgt\n+\n!!!!\n"
            "@1\nacgt\n+\n!!!!\n"
            "@2\nacgt\n+\n!!!!\n"
        );
        test_validate(dir);

        quick_gzip_write(spath,
            "@0\nacgt\n+\n\1\1\1\1\n"
            "@1\nacgt\n+\n!!!!\n"
            "@2\nacgt\n+\n!!!!\n"
        );
        expect_error("out-of-range quality score");
    }

    // Phred+64.
    {
        options.quality_type = sequence_string_set::QualityType::ILLUMINA64;
        sequence_string_set::mock(dir, 3, options);
        test_validate(dir);

        quick_gzip_write(spath,
            "@0\nacgt\n+\n@@@@\n"
            "@1\nacgt\n+\n@@@@\n"
            "@2\nacgt\n+\n@@@@\n"
        );
        test_validate(dir);

        quick_gzip_write(spath,
            "@0\nacgt\n+\n????\n"
            "@1\nacgt\n+\n@@@@\n"
            "@2\nacgt\n+\n@@@@\n"
        );
        expect_error("out-of-range quality score");
    }

    // Solexa+64.
    {
        options.quality_type = sequence_string_set::QualityType::SOLEXA;
        sequence_string_set::mock(dir, 3, options);
        test_validate(dir);

        quick_gzip_write(spath,
            "@0\nacgt\n+\n;;;;\n"
            "@1\nacgt\n+\n@@@@\n"
            "@2\nacgt\n+\n@@@@\n"
        );
        test_validate(dir);

        quick_gzip_write(spath,
            "@0\nacgt\n+\n::::\n"
            "@1\nacgt\n+\n@@@@\n"
            "@2\nacgt\n+\n@@@@\n"
        );
        expect_error("out-of-range quality score");
    }

    // None.
    {
        options.quality_type = sequence_string_set::QualityType::NONE;
        sequence_string_set::mock(dir, 10, options);
        quick_text_write((dir / "OBJECT").string(),
            "{ \"type\": \"sequence_string_set\", \"sequence_string_set\": { \"version\": \"1.0\", \"length\": 10, \"sequence_type\": \"RNA\", \"quality_type\": \"none\" } }"
        );
        test_validate(dir);
    }
}

TEST_F(SequenceStringSetTest, Names) {
    sequence_string_set::Options options;
    options.quality_type = sequence_string_set::QualityType::PHRED33;
    sequence_string_set::mock(dir, 10, options);

    auto npath = dir / "names.txt.gz";
    {
        byteme::GzipFileWriter writer(npath.c_str(), {});
        for (size_t i = 0; i < 10; ++i) {
            std::string tmpname = "\"gene_" + std::to_string(i + 1) + "\"\n";
            writer.write(reinterpret_cast<const unsigned char*>(tmpname.c_str()), tmpname.size());
        }
    }
    test_validate(dir);

    // Non-parallelized.
    {
        takane::Options inopt;
        inopt.parallel_reads = false;
        auto meta = takane::read_object_metadata(dir);
        takane::validate(dir, meta, inopt); 
    }

    // Works with newlines.
    {
        byteme::GzipFileWriter writer(npath.c_str(), {});
        for (size_t i = 0; i < 10; ++i) {
            std::string tmpname = "\"gene\n" + std::to_string(i + 1) + "\"\n";
            writer.write(reinterpret_cast<const unsigned char*>(tmpname.c_str()), tmpname.size());
        }
    }
    test_validate(dir);

    // Errors out with differences in counts.
    {
        byteme::GzipFileWriter writer(npath.c_str(), {});
        for (size_t i = 0; i < 5; ++i) {
            std::string tmpname = "\"gene-" + std::to_string(i + 1) + "\"\n";
            writer.write(reinterpret_cast<const unsigned char*>(tmpname.c_str()), tmpname.size());
        }
    }
    expect_error("number of names");

    // Errors out with quotes.
    {
        byteme::GzipFileWriter writer(npath.c_str(), {});
        for (size_t i = 0; i < 10; ++i) {
            std::string tmpname = "gene-" + std::to_string(i + 1) + "\n";
            writer.write(reinterpret_cast<const unsigned char*>(tmpname.c_str()), tmpname.size());
        }
    }
    expect_error("should start with a quote");

    {
        byteme::GzipFileWriter writer(npath.c_str(), {});
        for (size_t i = 0; i < 10; ++i) {
            std::string tmpname = "\"gene-" + std::to_string(i + 1) + "\"a\n";
            writer.write(reinterpret_cast<const unsigned char*>(tmpname.c_str()), tmpname.size());
        }
    }
    expect_error("present after end quote");

    // Errors out if there no terminating newline.
    {
        byteme::GzipFileWriter writer(npath.c_str(), {});
        for (size_t i = 0; i < 10; ++i) {
            std::string tmpname = "\"gene-" + std::to_string(i + 1) + "\"";
            if (i < 9) {
                tmpname += "\n";
            }
            writer.write(reinterpret_cast<const unsigned char*>(tmpname.c_str()), tmpname.size());
        }
    }
    expect_error("premature end");
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
    test_validate(dir);
}
