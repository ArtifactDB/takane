#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "utils.h"
#include "mock_sequence_string_set.h"
#include "mock_data_frame.h"
#include "mock_simple_list.h"

TEST(SequenceStringSet, VersionError) {
    auto dir = define_test_path("sequence_string_set");

    {
        initialize_directory_simple(dir, "sequence_string_set", "2.0");
    }
    expect_validation_error(dir, "unsupported version");
}

TEST(SequenceStringSet, LengthError) {
    auto dir = define_test_path("sequence_string_set");

    {
        initialize_directory(dir);
        quick_text_write(
            dir / "OBJECT",
            "{ \"type\": \"sequence_string_set\", \"sequence_string_set\": { \"version\": \"1.0\" } }"
        );
    }
    expect_validation_error(dir, "no such property");

    {
        initialize_directory(dir);
        quick_text_write(
            dir / "OBJECT",
            "{ \"type\": \"sequence_string_set\", \"sequence_string_set\": { \"version\": \"1.0\", \"length\": true } }"
        );
    }
    expect_validation_error(dir, "number");

    {
        initialize_directory(dir);
        quick_text_write(
            dir / "OBJECT",
            "{ \"type\": \"sequence_string_set\", \"sequence_string_set\": { \"version\": \"1.0\", \"length\": 1.5 } }"
        );
    }
    expect_validation_error(dir, "non-negative integer");

    {
        initialize_directory(dir);
        quick_text_write(
            dir / "OBJECT",
            "{ \"type\": \"sequence_string_set\", \"sequence_string_set\": { \"version\": \"1.0\", \"length\": -10 } }"
        );
    }
    expect_validation_error(dir, "non-negative integer");
}

TEST(SequenceStringSet, SequenceTypeError) {
    auto dir = define_test_path("sequence_string_set");

    {
        initialize_directory(dir);
        quick_text_write(
            dir / "OBJECT",
            "{ \"type\": \"sequence_string_set\", \"sequence_string_set\": { \"version\": \"1.0\", \"length\": 10 } }"
        );
    }
    expect_validation_error(dir, "not present");

    {
        initialize_directory(dir);
        quick_text_write(
            dir / "OBJECT",
            "{ \"type\": \"sequence_string_set\", \"sequence_string_set\": { \"version\": \"1.0\", \"length\": 10, \"sequence_type\": \"foobar\" } }"
        );
    }
    expect_validation_error(dir, "unknown sequence type 'foobar'");
}

TEST(SequenceStringSet, QualityTypeError) {
    auto dir = define_test_path("sequence_string_set");

    {
        initialize_directory(dir);
        quick_text_write(
            dir / "OBJECT",
            "{ \"type\": \"sequence_string_set\", \"sequence_string_set\": { \"version\": \"1.0\", \"length\": 10, \"sequence_type\": \"RNA\", \"quality_type\": true } }"
        );
    }
    expect_validation_error(dir, "expected a JSON string");

    {
        initialize_directory(dir);
        quick_text_write(
            dir / "OBJECT",
            "{ \"type\": \"sequence_string_set\", \"sequence_string_set\": { \"version\": \"1.0\", \"length\": 10, \"sequence_type\": \"RNA\", \"quality_type\": \"blah\" } }"
        );
    }
    expect_validation_error(dir, "unknown quality type 'blah'");

    {
        initialize_directory(dir);
        quick_text_write(
            dir / "OBJECT",
            "{ \"type\": \"sequence_string_set\", \"sequence_string_set\": { \"version\": \"1.0\", \"length\": 10, \"sequence_type\": \"RNA\", \"quality_type\": \"phred\" } }"
        );
    }
    expect_validation_error(dir, "no such property");

    {
        initialize_directory(dir);
        quick_text_write(
            dir / "OBJECT",
            "{ \"type\": \"sequence_string_set\", \"sequence_string_set\": { \"version\": \"1.0\", \"length\": 10, \"sequence_type\": \"RNA\", "
            "\"quality_type\": \"phred\", \"quality_offset\": true } }"
        );
    }
    expect_validation_error(dir, "should be a JSON number");

    {
        initialize_directory(dir);
        quick_text_write(
            dir / "OBJECT",
            "{ \"type\": \"sequence_string_set\", \"sequence_string_set\": { \"version\": \"1.0\", \"length\": 10, \"sequence_type\": \"RNA\", "
            "\"quality_type\": \"phred\", \"quality_offset\": 10 } }"
        );
    }
    expect_validation_error(dir, "33 or 64");
}

/****************************************/

TEST(SequenceStringSet, FastaBasicOkay) {
    auto dir = define_test_path("sequence_string_set");

    {
        // Use enough sequences so we get multiple digits in the names.
        SequenceStringSetOptions options(200);
        mock_sequence_string_set(dir, options);
    }

    test_validate(dir);
    EXPECT_EQ(test_height(dir), 200);

    // Repeating with parallelization.
    takane::Options opt;
    opt.parallel_reads = false;
    test_validate(dir, opt);
}

TEST(SequenceStringSet, FastaNewlinesOkay) {
    auto dir = define_test_path("sequence_string_set");
    std::string spath = dir / "sequences.fasta.gz";

    // Simulate annoying newlines everywhere.
    {
        initialize_sequence_string_set(dir, SequenceStringSetOptions(5));
        byteme::GzipFileWriter writer(spath.c_str(), {});
        dump_fasta(writer, 0, "AAAAAACCCCGGGGTTTT");
        dump_fasta(writer, 1, "\nAAAAAACCCCGGGGTTTT");
        dump_fasta(writer, 2, "AAAAAACCCCGGGGTTTT\n");
        dump_fasta(writer, 3, "AAAAAACCCC\nGGGGTTTT");
        dump_fasta(writer, 4, "AAAAAA\nCCCC\nGGGG\nTTTT");
    }
    {
        test_validate(dir);
        EXPECT_EQ(test_height(dir), 5);
    }

    // Check correct chomping.
    {
        initialize_sequence_string_set(dir, SequenceStringSetOptions(4));
        byteme::GzipFileWriter writer(spath.c_str(), {});
        std::string newlines = "\n\n\n";
        writer.write(reinterpret_cast<const unsigned char*>(newlines.c_str()), newlines.size());
        dump_fasta(writer, 0, "AAAAAACCCCGGGGTTTT");
        dump_fasta(writer, 1, "\n\nAAAAA\nACCCCGG\nGGTTTT");
        dump_fasta(writer, 2, "AAAAA\n\nACCCCGG\n\nGGTTTT");
        dump_fasta(writer, 3, "AAAAA\nACCCCGG\nGGTTTT");
        writer.write(reinterpret_cast<const unsigned char*>(newlines.c_str()), newlines.size());
    }
    {
        test_validate(dir);
        EXPECT_EQ(test_height(dir), 4);
    }
}

TEST(SequenceStringSet, FastaEmptyOkay) {
    auto dir = define_test_path("sequence_string_set");
    std::string spath = dir / "sequences.fasta.gz";

    // Zero-length sequences are okay.
    {
        initialize_sequence_string_set(dir, SequenceStringSetOptions(2));
        quick_gzip_write(spath,
            ">0\n\n"
            ">1\n\n"
        );
    }
    test_validate(dir);

    // Empty file is okay.
    {
        initialize_sequence_string_set(dir, SequenceStringSetOptions(0));
        quick_gzip_write(spath, "");
    }
    test_validate(dir);
    EXPECT_EQ(test_height(dir), 0);

    // Newline-only file is okay.
    {
        initialize_sequence_string_set(dir, SequenceStringSetOptions(0));
        quick_gzip_write(spath, "\n\n\n");
    }
    test_validate(dir);
    EXPECT_EQ(test_height(dir), 0);
}

TEST(SequenceStringSet, FastaNameError) {
    auto dir = define_test_path("sequence_string_set");
    std::string spath = dir / "sequences.fasta.gz";

    {
        initialize_sequence_string_set(dir, SequenceStringSetOptions(5));
        quick_gzip_write(spath, "foobar\n");
    }
    expect_validation_error(dir, "sequence name should start with '>'");

    {
        initialize_sequence_string_set(dir, SequenceStringSetOptions(5));
        quick_gzip_write(spath, ">foobar\n");
    }
    expect_validation_error(dir, "sequence name should be a non-negative integer");

    {
        initialize_sequence_string_set(dir, SequenceStringSetOptions(50));
        quick_gzip_write(spath, ">123\n");
    }
    expect_validation_error(dir, "out of bounds"); // fails the LHS condition of the out-of-bounds check.

    {
        initialize_sequence_string_set(dir, SequenceStringSetOptions(52));
        quick_gzip_write(spath, ">53\n");
    }
    expect_validation_error(dir, "out of bounds"); // fails the RHS condition of the out-of-bounds check.

    {
        initialize_sequence_string_set(dir, SequenceStringSetOptions(5));
        quick_gzip_write(spath, ">\n"); // Empty name.
    }
    expect_validation_error(dir, "sequence name should be its index");

    {
        initialize_sequence_string_set(dir, SequenceStringSetOptions(5));
        byteme::GzipFileWriter writer(spath.c_str(), {});
        for (std::size_t i = 0; i < 200; ++i) {
            dump_fasta(writer, i % 10, "ACGT");
        }
    }
    expect_validation_error(dir, "sequence name should be its index");
}

TEST(SequenceStringSet, FastaPrematureEndError) {
    auto dir = define_test_path("sequence_string_set");
    std::string spath = dir / "sequences.fasta.gz";

    std::vector<std::string> contents{ ">1", ">1\n", ">1\nACGT" };
    for (const auto& x : contents) {
        {
            initialize_sequence_string_set(dir, SequenceStringSetOptions(2));
            quick_gzip_write(spath, ">0\nACGT\n" + x);
        }
        expect_validation_error(dir, "premature end");
    }
}

TEST(SequenceStringSet, FastaCountError) {
    auto dir = define_test_path("sequence_string_set");
    std::string spath = dir / "sequences.fasta.gz";

    // Checking the count.
    {
        initialize_sequence_string_set(dir, SequenceStringSetOptions(2));
        byteme::GzipFileWriter writer(spath.c_str(), {});
        dump_fasta(writer, 0, "AAAAAACCCCGGGGTTTT");
    }
    expect_validation_error(dir, "number of sequences");
}

/****************************************/

TEST(SequenceStringSet, FastqBasicOkay) {
    auto dir = define_test_path("sequence_string_set");

    {
        SequenceStringSetOptions options(300);
        options.quality_type = QualityType::PHRED33;
        mock_sequence_string_set(dir, options);
    }

    test_validate(dir);
    EXPECT_EQ(test_height(dir), 300);

    // Non-parallelized.
    takane::Options inopt;
    inopt.parallel_reads = false;
    test_validate(dir, inopt); 
}

TEST(SequenceStringSet, FastqPlusOkay) {
    auto dir = define_test_path("sequence_string_set");
    std::string spath = dir / "sequences.fastq.gz";

    // Check that the validator ignores gunk on the + line.
    {
        SequenceStringSetOptions options(2);
        options.quality_type = QualityType::PHRED33;
        initialize_sequence_string_set(dir, options);
        quick_gzip_write(spath,
            "@0\nACGT\n+123123123\nFECD\n"
            "@1\nACGT\n+ foo bar\nFECD\n"
        );
    }

    test_validate(dir);
    EXPECT_EQ(test_height(dir), 2);
}

TEST(SequenceStringSet, FastqAtOkay) {
    auto dir = define_test_path("sequence_string_set");
    std::string spath = dir / "sequences.fastq.gz";

    // Check that the validator handles '@' in the quality scores.
    {
        SequenceStringSetOptions options(2);
        options.quality_type = QualityType::PHRED33;
        initialize_sequence_string_set(dir, options);
        quick_gzip_write(spath,
            "@0\nACGT\n+\n@@@@\n"
            "@1\nACGT\n+\n++++\n"
        );
    }

    test_validate(dir);
    EXPECT_EQ(test_height(dir), 2);
}

TEST(SequenceStringSet, FastqNewlinesOkay) {
    auto dir = define_test_path("sequence_string_set");
    std::string spath = dir / "sequences.fastq.gz";
 
    // Simulate annoying newlines everywhere.
    {
        SequenceStringSetOptions options(12);
        options.quality_type = QualityType::PHRED33;
        initialize_sequence_string_set(dir, options);
        byteme::GzipFileWriter writer(spath.c_str(), {});
        dump_fastq(writer, 0, "AAAACCCCGGGGTTTT", "FFFFEEEEDDDDCCCC");
        dump_fastq(writer, 1, "\nAAAACCCCGGGGTTTT", "FFFFEEEEDDDDCCCC");
        dump_fastq(writer, 2, "AAAACCCCGGGGTTTT\n", "FFFFEEEEDDDDCCCC");
        dump_fastq(writer, 3, "AAAACCCC\nGGGGTTTT", "FFFFEEEEDDDDCCCC");
        dump_fastq(writer, 4, "AAAA\nCCCC\nGGGG\nTTTT", "FFFFEEEEDDDDCCCC");
        dump_fastq(writer, 5, "AAAACCCC\n\nGGGGTTTT", "FFFFEEEEDDDDCCCC");
        dump_fastq(writer, 6, "AAAACCCCGGGGTTTT", "\nFFFFEEEEDDDDCCCC");
        dump_fastq(writer, 7, "AAAACCCCGGGGTTTT", "FFFFEEEEDDDDCCCC\n");
        dump_fastq(writer, 8, "AAAACCCCGGGGTTTT", "FFFFEEEEDDDD\nCCCC");
        dump_fastq(writer, 9, "AAAACCCCGGGGTTTT", "FFFFEEEE\nDDDDCCCC");
        dump_fastq(writer, 10, "AAAACCCCGGGGTTTT", "FFFFEEEE\n\nDDDDCCCC");
        dump_fastq(writer, 11, "AAAACCCCGGGGTTTT", "FFFF\nEEEE\nDDDD\nCCCC");
    }
    {
        test_validate(dir);
        EXPECT_EQ(test_height(dir), 12);
    }

    // Chomps work correctly.
    {
        SequenceStringSetOptions options(2);
        options.quality_type = QualityType::PHRED33;
        initialize_sequence_string_set(dir, options);
        quick_gzip_write(spath,
            "\n\n\n"
            "@0\nACGTACGTACGT\n+\n@@@@@@@@@@@@\n"
            "@1\nACGT\n+\n++++\n"
            "\n\n"
        );
    }
    {
        test_validate(dir);
        EXPECT_EQ(test_height(dir), 2);
    }
}

TEST(SequenceStringSet, FastqEmptyOkay) {
    auto dir = define_test_path("sequence_string_set");
    std::string spath = dir / "sequences.fastq.gz";

    // Zero length sequences are okay.
    {
        SequenceStringSetOptions options(2);
        options.quality_type = QualityType::PHRED33;
        initialize_sequence_string_set(dir, options);
        quick_gzip_write(spath,
            "@0\n\n+\n\n"
            "@1\n\n+\n\n"
        );
    }
    {
        test_validate(dir);
        EXPECT_EQ(test_height(dir), 2);
    }

    // Empty file is okay.
    {
        SequenceStringSetOptions options(0);
        options.quality_type = QualityType::PHRED33;
        initialize_sequence_string_set(dir, options);
        quick_gzip_write(spath, "");
    }
    {
        test_validate(dir);
        EXPECT_EQ(test_height(dir), 0);
    }

    // Newline-only file is okay.
    {
        SequenceStringSetOptions options(0);
        options.quality_type = QualityType::PHRED33;
        initialize_sequence_string_set(dir, options);
        quick_gzip_write(spath, "\n\n\n");
    }
    {
        test_validate(dir);
        EXPECT_EQ(test_height(dir), 0);
    }
}

TEST(SequenceStringSet, FastqNameError) {
    auto dir = define_test_path("sequence_string_set");
    std::string spath = dir / "sequences.fastq.gz";

    SequenceStringSetOptions options(2);
    options.quality_type = QualityType::PHRED33;

    {
        initialize_sequence_string_set(dir, options);
        quick_gzip_write(spath, "foobar\n");
    }
    expect_validation_error(dir, "sequence name should start with '@'");

    {
        initialize_sequence_string_set(dir, options);
        quick_gzip_write(spath, "@foobar\n");
    }
    expect_validation_error(dir, "sequence name should be a non-negative integer");

    {
        initialize_sequence_string_set(dir, options);
        quick_gzip_write(spath, "@\n");
    }
    expect_validation_error(dir, "sequence name should be its index");

    {
        SequenceStringSetOptions options(200);
        options.quality_type = QualityType::PHRED33;
        initialize_sequence_string_set(dir, options);
        byteme::GzipFileWriter writer(spath.c_str(), {});
        for (size_t i = 0; i < 200; ++i) {
            dump_fastq(writer, i % 10, "ACGT", "FECD");
        }
    }
    expect_validation_error(dir, "sequence name should be its index");
}

TEST(SequenceStringSet, FastqLengthMismatchError) {
    auto dir = define_test_path("sequence_string_set");
    std::string spath = dir / "sequences.fastq.gz";

    SequenceStringSetOptions options(2);
    options.quality_type = QualityType::PHRED33;

    // Fails with a mismatch in the lengths.
    {
        initialize_sequence_string_set(dir, options);
        quick_gzip_write(spath,
            "@0\nACGT\n+\n@@@\n"
            "@1\nACGT\n+\n++++\n"
        );
    }
    expect_validation_error(dir, "unequal lengths");

    // More complicated mismatch in the lengths.
    {
        initialize_sequence_string_set(dir, options);
        quick_gzip_write(spath,
            "@0\nACGTACGTACGT\n+\n@@@@\n@@@@\n@\n@@@@\n"
            "@1\nACGT\n+\n++++\n"
        );
    }
    expect_validation_error(dir, "unequal lengths");
}

TEST(SequenceStringSet, FastqPrematureEndError) {
    auto dir = define_test_path("sequence_string_set");
    std::string spath = dir / "sequences.fastq.gz";

    SequenceStringSetOptions options(1);
    options.quality_type = QualityType::PHRED33;

    std::vector<std::string> contents{
        "@0",
        "@0\n",
        "@0\nACGT",
        "@0\nACGT\n",
        "@0\nACGT\n+",
        "@0\nACGT\n+\n",
        "@0\nACGT\n+\n++++"
    };

    for (const auto& x : contents) {
        {
            initialize_sequence_string_set(dir, options);
            quick_gzip_write(spath, x);
        }
        expect_validation_error(dir, "premature end");
    }
}

TEST(SequenceStringSet, FastqCountError) {
    auto dir = define_test_path("sequence_string_set");
    std::string spath = dir / "sequences.fastq.gz";

    {
        SequenceStringSetOptions options(0);
        options.quality_type = QualityType::PHRED33;
        initialize_sequence_string_set(dir, options);
        byteme::GzipFileWriter writer(spath.c_str(), {});
        dump_fastq(writer, 0, "AAAACCCCGGGGTTTT", "~~~~!!!!AAAAFFFF");
    }

    expect_validation_error(dir, "number of sequences");
}

/****************************************/

TEST(SequenceStringSet, DnaOkay) {
    auto dir = define_test_path("sequence_string_set");
    std::string spath = dir / "sequences.fasta.gz";

    {
        SequenceStringSetOptions options(150);
        mock_sequence_string_set(dir, options);
    }
    {
        test_validate(dir);
        EXPECT_EQ(test_height(dir), 150);
    }

    // More exotic choices.
    {
        SequenceStringSetOptions options(3);
        options.sequence_type = SequenceType::DNA;
        initialize_sequence_string_set(dir, options);
        quick_gzip_write(
            dir / "sequences.fasta.gz",
            ">0\nacgt\n"
            ">1\na.c-g.tacgryswkmbdhvn.-\n"
            ">2\nTACGRYSWKMBDHVN.-\n"
        );
    }
    {
        test_validate(dir);
        EXPECT_EQ(test_height(dir), 3);
    }
}

TEST(SequenceStringSet, DnaError) {
    auto dir = define_test_path("sequence_string_set");

    {
        SequenceStringSetOptions options(3);
        initialize_sequence_string_set(dir, options);
        quick_gzip_write(
            dir / "sequences.fasta.gz",
            ">0\nacgt\n"
            ">1\nuuuu\n"
            ">2\nACGT\n"
        );
    }
    expect_validation_error(dir, "forbidden character 'u'");

    // Repeat for FASTQ, just to ensure that the same checks are run.
    {
        SequenceStringSetOptions options(1);
        options.quality_type = QualityType::PHRED33;
        initialize_sequence_string_set(dir, options);
        quick_gzip_write(
            dir / "sequences.fastq.gz",
            "@0\nacgx\n+\n;;;;\n"
        );
    }
    expect_validation_error(dir, "forbidden character 'x'");
}

TEST(SequenceStringSet, RnaOkay) {
    auto dir = define_test_path("sequence_string_set");

    {
        SequenceStringSetOptions options(201);
        options.sequence_type = SequenceType::RNA;
        mock_sequence_string_set(dir, options);
    }
    {
        test_validate(dir);
        EXPECT_EQ(test_height(dir), 201);
    }

    // Trying with some more exotic bases.
    {
        SequenceStringSetOptions options(3);
        options.sequence_type = SequenceType::RNA;
        initialize_sequence_string_set(dir, options);
        quick_gzip_write(
            dir / "sequences.fasta.gz",
            ">0\nacgU\n"
            ">1\na.c-g.uacgryswkmbdhvn.-\n"
            ">2\nUACGRYSWKMBDHVN.-\n"
        );
    }
    {
        test_validate(dir);
        EXPECT_EQ(test_height(dir), 3);
    }
}

TEST(SequenceStringSet, RnaError) {
    auto dir = define_test_path("sequence_string_set");

    {
        SequenceStringSetOptions options(3);
        options.sequence_type = SequenceType::RNA;
        initialize_sequence_string_set(dir, options);
        quick_gzip_write(
            dir / "sequences.fasta.gz",
            ">0\nacgu\n"
            ">1\nTTTT\n"
            ">2\nACGU\n"
        );
    }
    expect_validation_error(dir, "forbidden character 'T'");
}

TEST(SequenceStringSet, ProteinOkay) {
    auto dir = define_test_path("sequence_string_set");

    {
        SequenceStringSetOptions options(234);
        options.sequence_type = SequenceType::AA;
        mock_sequence_string_set(dir, options);
    }
    {
        test_validate(dir);
        EXPECT_EQ(test_height(dir), 234);
    }

    // Trying with some more exotic bases.
    {
        SequenceStringSetOptions options(3);
        options.sequence_type = SequenceType::AA;
        mock_sequence_string_set(dir, options);
        quick_gzip_write(
            dir / "sequences.fasta.gz",
            ">0\nacgt\n"
            ">1\nACD.EFGHIKLMNP-QRSTVWY\n"
            ">2\nacd.efghiklmnp-qrstvwy\n"
        );
    }
    {
        test_validate(dir);
        EXPECT_EQ(test_height(dir), 3);
    }
}

TEST(SequenceStringSet, ProteinError) {
    auto dir = define_test_path("sequence_string_set");

    {
        SequenceStringSetOptions options(3);
        options.sequence_type = SequenceType::AA;
        initialize_sequence_string_set(dir, options);
        quick_gzip_write(
            dir / "sequences.fasta.gz",
            ">0\nacgt\n"
            ">1\nxxxx\n"
            ">2\nACGU\n"
        );
    }
    expect_validation_error(dir, "forbidden character 'x'");
}

TEST(SequenceStringSet, CustomOkay) {
    auto dir = define_test_path("sequence_string_set");

    {
        SequenceStringSetOptions options(99);
        options.sequence_type = SequenceType::CUSTOM;
        mock_sequence_string_set(dir, options);
    }
    {
        test_validate(dir);
        EXPECT_EQ(test_height(dir), 99);
    }

    {
        SequenceStringSetOptions options(3);
        options.sequence_type = SequenceType::CUSTOM;
        initialize_sequence_string_set(dir, options);
        quick_gzip_write(
            dir / "sequences.fasta.gz",
            ">0\nxxxx\n"
            ">1\n!;;;*8acac~\n"
            ">2\nacd.efghiklmnp-qrstvwy\n"
        );
    }
    {
        test_validate(dir);
        EXPECT_EQ(test_height(dir), 3);
    }
}

/****************************************/

TEST(SequenceStringSet, Phred33Okay) {
    auto dir = define_test_path("sequence_string_set");

    {
        SequenceStringSetOptions options(101);
        options.quality_type = QualityType::PHRED33;
        mock_sequence_string_set(dir, options);
    }
    {
        test_validate(dir);
        EXPECT_EQ(test_height(dir), 101);
    }

    // Extremes of qualities are okay.
    {
        SequenceStringSetOptions options(3);
        options.quality_type = QualityType::PHRED33;
        initialize_sequence_string_set(dir, options);
        quick_gzip_write(
            dir / "sequences.fastq.gz",
            "@0\nacgt\n+\n!~!~\n"
            "@1\nacgt\n+\n~!~!\n"
            "@2\nacgt\n+\n!~~!\n"
        );
    }
    {
        test_validate(dir);
        EXPECT_EQ(test_height(dir), 3);
    }
}

TEST(SequenceStringSet, Phred33Error) {
    auto dir = define_test_path("sequence_string_set");

    {
        SequenceStringSetOptions options(3);
        options.quality_type = QualityType::PHRED33;
        initialize_sequence_string_set(dir, options);
        quick_gzip_write(
            dir / "sequences.fastq.gz",
            "@0\nacgt\n+\n\1\1\1\1\n"
            "@1\nacgt\n+\n!!!!\n"
            "@2\nacgt\n+\n!!!!\n"
        );
    }
    expect_validation_error(dir, "out-of-range quality score");
}

TEST(SequenceStringSet, Phred64Okay) {
    auto dir = define_test_path("sequence_string_set");

    {
        SequenceStringSetOptions options(59);
        options.quality_type = QualityType::ILLUMINA64;
        mock_sequence_string_set(dir, options);
    }
    {
        test_validate(dir);
        EXPECT_EQ(test_height(dir), 59);
    }

    // Checking extremes.
    {
        SequenceStringSetOptions options(3);
        options.quality_type = QualityType::ILLUMINA64;
        initialize_sequence_string_set(dir, options);
        quick_gzip_write(
            dir / "sequences.fastq.gz",
            "@0\nacgt\n+\n@@@@\n"
            "@1\nacgt\n+\n@~~@\n"
            "@2\nacgt\n+\n~@@~\n"
        );
    }
    {
        test_validate(dir);
        EXPECT_EQ(test_height(dir), 3);
    }
}

TEST(SequenceStringSet, Phred64Error) {
    auto dir = define_test_path("sequence_string_set");

    {
        SequenceStringSetOptions options(3);
        options.quality_type = QualityType::ILLUMINA64;
        initialize_sequence_string_set(dir, options);
        quick_gzip_write(
            dir / "sequences.fastq.gz",
            "@0\nacgt\n+\n????\n"
            "@1\nacgt\n+\n@@@@\n"
            "@2\nacgt\n+\n@@@@\n"
        );
    }
    expect_validation_error(dir, "out-of-range quality score");
}

TEST(SequenceStringSet, SolexaOkay) {
    auto dir = define_test_path("sequence_string_set");

    {
        SequenceStringSetOptions options(24);
        options.quality_type = QualityType::SOLEXA;
        mock_sequence_string_set(dir, options);
    }
    {
        test_validate(dir);
        EXPECT_EQ(test_height(dir), 24);
    }

    {
        SequenceStringSetOptions options(3);
        options.quality_type = QualityType::SOLEXA;
        initialize_sequence_string_set(dir, options);
        quick_gzip_write(
            dir / "sequences.fastq.gz",
            "@0\nacgt\n+\n;;;;\n"
            "@1\nacgt\n+\n@@@@\n"
            "@2\nacgt\n+\n@@@@\n"
        );
    }
    {
        test_validate(dir);
        EXPECT_EQ(test_height(dir), 3);
    }
}

TEST(SequenceStringSet, SolexaError) {
    auto dir = define_test_path("sequence_string_set");

    {
        SequenceStringSetOptions options(3);
        options.quality_type = QualityType::SOLEXA;
        initialize_sequence_string_set(dir, options);
        quick_gzip_write(
            dir / "sequences.fastq.gz",
            "@0\nacgt\n+\n::::\n"
            "@1\nacgt\n+\n@@@@\n"
            "@2\nacgt\n+\n@@@@\n"
        );
    }
    expect_validation_error(dir, "out-of-range quality score");
}

TEST(SequenceStringSet, NoneOkay) {
    auto dir = define_test_path("sequence_string_set");

    {
        SequenceStringSetOptions options(10);
        options.quality_type = QualityType::NONE;
        mock_sequence_string_set(dir, options);
        // Manually inject 'none' in the OBJECT metadata.
        quick_text_write(
            dir / "OBJECT",
            "{ \"type\": \"sequence_string_set\", \"sequence_string_set\": { \"version\": \"1.0\", \"length\": 10, \"sequence_type\": \"RNA\", \"quality_type\": \"none\" } }"
        );
    }
    test_validate(dir);
    EXPECT_EQ(test_height(dir), 10);
}

/****************************************/

TEST(SequenceStringSet, NamesOkay) {
    auto dir = define_test_path("sequence_string_set");
    std::string npath = dir / "names.txt.gz";

    {
        SequenceStringSetOptions options(10);
        mock_sequence_string_set(dir, options);
        byteme::GzipFileWriter writer(npath.c_str(), {});
        for (int i = 0; i < 10; ++i) {
            std::string tmpname = "\"gene_" + std::to_string(i + 1) + "\"\n";
            writer.write(reinterpret_cast<const unsigned char*>(tmpname.c_str()), tmpname.size());
        }
    }
    {
        test_validate(dir);
        EXPECT_EQ(test_height(dir), 10);

        // Non-parallelized.
        takane::Options inopt;
        inopt.parallel_reads = false;
        test_validate(dir, inopt); 
    }

    // Works with quotes in the names.
    {
        SequenceStringSetOptions options(10);
        mock_sequence_string_set(dir, options);
        byteme::GzipFileWriter writer(npath.c_str(), {});
        for (int i = 0; i < 10; ++i) {
            std::string tmpname = "\"gene\"\"" + std::to_string(i + 1) + "\"\n";
            writer.write(reinterpret_cast<const unsigned char*>(tmpname.c_str()), tmpname.size());
        }
    }
    {
        test_validate(dir);
        EXPECT_EQ(test_height(dir), 10);
    }

    // Works with newlines in the names.
    {
        SequenceStringSetOptions options(10);
        mock_sequence_string_set(dir, options);
        byteme::GzipFileWriter writer(npath.c_str(), {});
        for (int i = 0; i < 10; ++i) {
            std::string tmpname = "\"gene\n" + std::to_string(i + 1) + "\"\n";
            writer.write(reinterpret_cast<const unsigned char*>(tmpname.c_str()), tmpname.size());
        }
    }
    {
        test_validate(dir);
        EXPECT_EQ(test_height(dir), 10);
    }
}

TEST(SequenceStringSet, NamesError) {
    auto dir = define_test_path("sequence_string_set");
    std::string npath = dir / "names.txt.gz";

    // Difference in counts.
    {
        SequenceStringSetOptions options(10);
        mock_sequence_string_set(dir, options);
        byteme::GzipFileWriter writer(npath.c_str(), {});
        for (int i = 0; i < 5; ++i) {
            std::string tmpname = "\"gene-" + std::to_string(i + 1) + "\"\n";
            writer.write(reinterpret_cast<const unsigned char*>(tmpname.c_str()), tmpname.size());
        }
    }
    expect_validation_error(dir, "number of names");

    // No quotes.
    {
        SequenceStringSetOptions options(10);
        mock_sequence_string_set(dir, options);
        byteme::GzipFileWriter writer(npath.c_str(), {});
        for (int i = 0; i < 10; ++i) {
            std::string tmpname = "gene-" + std::to_string(i + 1) + "\n";
            writer.write(reinterpret_cast<const unsigned char*>(tmpname.c_str()), tmpname.size());
        }
    }
    expect_validation_error(dir, "should start with a quote");

    // Unescaped quotes.
    {
        SequenceStringSetOptions options(10);
        mock_sequence_string_set(dir, options);
        byteme::GzipFileWriter writer(npath.c_str(), {});
        for (size_t i = 0; i < 10; ++i) {
            std::string tmpname = "\"gene-" + std::to_string(i + 1) + "\"a\n";
            writer.write(reinterpret_cast<const unsigned char*>(tmpname.c_str()), tmpname.size());
        }
    }
    expect_validation_error(dir, "present after end quote");

    // No terminating newline.
    {
        SequenceStringSetOptions options(10);
        mock_sequence_string_set(dir, options);
        byteme::GzipFileWriter writer(npath.c_str(), {});
        for (size_t i = 0; i < 10; ++i) {
            std::string tmpname = "\"gene-" + std::to_string(i + 1) + "\"";
            if (i < 9) {
                tmpname += "\n";
            }
            writer.write(reinterpret_cast<const unsigned char*>(tmpname.c_str()), tmpname.size());
        }
    }
    expect_validation_error(dir, "premature end");
}

/****************************************/

TEST(SequenceStringSet, McolsOkay) {
    auto dir = define_test_path("sequence_string_set");

    {
        SequenceStringSetOptions options(20);
        mock_sequence_string_set(dir, options);
        mock_data_frame(dir / "sequence_annotations", DataFrameOptions(20, {}));
    }

    test_validate(dir);
    EXPECT_EQ(test_height(dir), 20);
}

TEST(SequenceStringSet, McolsError) {
    auto dir = define_test_path("sequence_string_set");

    {
        SequenceStringSetOptions options(20);
        mock_sequence_string_set(dir, options);
        mock_data_frame(dir / "sequence_annotations", DataFrameOptions(10, {}));
    }
    expect_validation_error(dir, "number of rows");
}

TEST(SequenceStringSet, MetadataOkay) {
    auto dir = define_test_path("sequence_string_set");

    {
        SequenceStringSetOptions options(20);
        mock_sequence_string_set(dir, options);
        mock_simple_list(dir / "other_annotations");
    }

    test_validate(dir);
    EXPECT_EQ(test_height(dir), 20);
}

TEST(SequenceStringSet, MetadataError) {
    auto dir = define_test_path("sequence_string_set");

    {
        SequenceStringSetOptions options(20);
        mock_sequence_string_set(dir, options);
        initialize_directory_simple(dir / "other_annotations", "data_frame", "1.0");
    }
    expect_validation_error(dir, "SIMPLE_LIST");
}
