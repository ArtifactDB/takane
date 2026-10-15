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

    // Simulate annoying newlines everywhere.
    {
        initialize_sequence_string_set(dir, SequenceStringSetOptions(5));
        std::string spath = dir / "sequences.fasta.gz";
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
        std::string spath = dir / "sequences.fasta.gz";
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
    auto spath = dir / "sequences.fasta.gz";

    // Zero-length sequences are okay.
    {
        initialize_sequence_string_set(dir, SequenceStringSetOptions(2));
        quick_gzip_write(spath.string(),
            ">0\n\n"
            ">1\n\n"
        );
    }
    test_validate(dir);

    // Empty file is okay.
    {
        initialize_sequence_string_set(dir, SequenceStringSetOptions(0));
        quick_gzip_write(spath.string(), "");
    }
    test_validate(dir);
    EXPECT_EQ(test_height(dir), 0);

    // Empty file with only newlines is okay.
    {
        initialize_sequence_string_set(dir, SequenceStringSetOptions(0));
        quick_gzip_write(spath.string(), "\n\n\n");
    }
    test_validate(dir);
    EXPECT_EQ(test_height(dir), 0);
}

TEST(SequenceStringSet, FastaNameError) {
    auto dir = define_test_path("sequence_string_set");
    auto spath = dir / "sequences.fasta.gz";

    {
        initialize_sequence_string_set(dir, SequenceStringSetOptions(5));
        quick_gzip_write(spath.string(), "foobar\n");
    }
    expect_validation_error(dir, "sequence name should start with '>'");

    {
        initialize_sequence_string_set(dir, SequenceStringSetOptions(5));
        quick_gzip_write(spath.string(), ">foobar\n");
    }
    expect_validation_error(dir, "sequence name should be a non-negative integer");

    {
        initialize_sequence_string_set(dir, SequenceStringSetOptions(50));
        quick_gzip_write(spath.string(), ">123\n");
    }
    expect_validation_error(dir, "out of bounds"); // fails the LHS condition of the out-of-bounds check.

    {
        initialize_sequence_string_set(dir, SequenceStringSetOptions(52));
        quick_gzip_write(spath.string(), ">53\n");
    }
    expect_validation_error(dir, "out of bounds"); // fails the RHS condition of the out-of-bounds check.

    {
        initialize_sequence_string_set(dir, SequenceStringSetOptions(5));
        quick_gzip_write(spath.string(), ">\n"); // Empty name.
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

TEST(SequenceStringSet, FastaStructuralError) {
    auto dir = define_test_path("sequence_string_set");
    auto spath = dir / "sequences.fasta.gz";

    {
        initialize_sequence_string_set(dir, SequenceStringSetOptions(2));
        quick_gzip_write(spath.string(),
            ">0\nACGT\n"
            ">1\n" // no terminating newline.
        );
    }
    expect_validation_error(dir, "premature end");

    // Checking the count.
    {
        initialize_sequence_string_set(dir, SequenceStringSetOptions(2));
        byteme::GzipFileWriter writer(spath.c_str(), {});
        dump_fasta(writer, 0, "AAAAAACCCCGGGGTTTT");
    }
    expect_validation_error(dir, "number of sequences");
}

//TEST_F(SequenceStringSetTest, FastqParsing) {
//    sequence_string_set::Options options;
//    options.quality_type = sequence_string_set::QualityType::PHRED33;
//    sequence_string_set::mock(dir, 200, options);
//    test_validate(dir); // OKAY.
//
//    // Non-parallelized.
//    {
//        takane::Options inopt;
//        inopt.parallel_reads = false;
//        auto meta = takane::read_object_metadata(dir);
//        takane::validate(dir, meta, inopt); 
//    }
//
//    // Name checks.
//    auto spath = dir / "sequences.fastq.gz";
//    {
//        quick_gzip_write(spath.string(), "foobar\n");
//        expect_error("sequence name should start with '@'");
//
//        quick_gzip_write(spath.string(), "@foobar\n");
//        expect_error("sequence name should be a non-negative integer");
//
//        quick_gzip_write(spath.string(), "@\n");
//        expect_error("sequence name should be its index");
//
//        {
//            byteme::GzipFileWriter writer(spath.c_str(), {});
//            for (size_t i = 0; i < 200; ++i) {
//                sequence_string_set::dump_fastq(writer, i % 10, "ACGT", "FECD");
//            }
//        }
//        expect_error("sequence name should be its index");
//    }
//
//    // Structural checks for correct parsing.
//    {
//        sequence_string_set::mock(dir, 2, options);
//
//        // Ignores gunk on the + line.
//        quick_gzip_write(spath.string(),
//            "@0\nACGT\n+123123123\nFECD\n"
//            "@1\nACGT\n+ foo bar\nFECD\n"
//        );
//        test_validate(dir);
//
//        // Works when you have '@' in the quality scores.
//        quick_gzip_write(spath.string(),
//            "@0\nACGT\n+\n@@@@\n"
//            "@1\nACGT\n+\n++++\n"
//        );
//        test_validate(dir);
//
//        // Fails with a mismatch in the lengths.
//        quick_gzip_write(spath.string(),
//            "@0\nACGT\n+\n@@@\n"
//            "@1\nACGT\n+\n++++\n"
//        );
//        expect_error("unequal lengths");
//
//        quick_gzip_write(spath.string(),
//            "@0\nACGTACGTACGT\n+\n@@@@\n@@@@\n@@@@\n"
//            "@1\nACGT\n+\n++++\n"
//        );
//        test_validate(dir); // OK!
//
//        // More complicated mismatch in the lengths.
//        quick_gzip_write(spath.string(),
//            "@0\nACGTACGTACGT\n+\n@@@@\n@@@@\n@\n@@@@\n"
//            "@1\nACGT\n+\n++++\n"
//        );
//        expect_error("unequal lengths");
//
//        // Zero length sequences are okay.
//        quick_gzip_write(spath.string(),
//            "@0\n\n+\n\n"
//            "@1\n\n+\n\n"
//        );
//        test_validate(dir);
//
//        // But they must be newline-terminated.
//        quick_gzip_write(spath.string(),
//            "@0\n\n+\n\n"
//            "@1\n\n+\n"
//        );
//        expect_error("premature end");
//    }
//
//    // Checking the count.
//    sequence_string_set::mock(dir, 5, options);
//    {
//        byteme::GzipFileWriter writer(spath.c_str(), {});
//        sequence_string_set::dump_fastq(writer, 0, "AAAACCCCGGGGTTTT", "~~~~!!!!AAAAFFFF");
//    }
//    expect_error("observed number of sequences");
//
//    // Simulating sequences with annoying newlines everywhere.
//    sequence_string_set::mock(dir, 10, options);
//    {
//        byteme::GzipFileWriter writer(spath.c_str(), {});
//        sequence_string_set::dump_fastq(writer, 0, "AAAACCCCGGGGTTTT", "FFFFEEEEDDDDCCCC");
//        sequence_string_set::dump_fastq(writer, 1, "\nAAAACCCCGGGGTTTT", "FFFFEEEEDDDDCCCC");
//        sequence_string_set::dump_fastq(writer, 2, "AAAACCCCGGGGTTTT\n", "FFFFEEEEDDDDCCCC");
//        sequence_string_set::dump_fastq(writer, 3, "AAAACCCC\nGGGGTTTT", "FFFFEEEEDDDDCCCC");
//        sequence_string_set::dump_fastq(writer, 4, "AAAA\nCCCC\nGGGG\nTTTT", "FFFFEEEEDDDDCCCC");
//        sequence_string_set::dump_fastq(writer, 5, "AAAACCCCGGGGTTTT", "\nFFFFEEEEDDDDCCCC");
//        sequence_string_set::dump_fastq(writer, 6, "AAAACCCCGGGGTTTT", "FFFFEEEEDDDDCCCC\n");
//        sequence_string_set::dump_fastq(writer, 7, "AAAACCCCGGGGTTTT", "FFFFEEEEDDDD\nCCCC");
//        sequence_string_set::dump_fastq(writer, 8, "AAAACCCCGGGGTTTT", "FFFFEEEE\nDDDDCCCC");
//        sequence_string_set::dump_fastq(writer, 9, "AAAACCCCGGGGTTTT", "FFFF\nEEEE\nDDDD\nCCCC");
//    }
//    test_validate(dir); // OKAY.
//}
//
//TEST_F(SequenceStringSetTest, SequenceTypes) {
//    sequence_string_set::Options options;
//    auto spath = (dir / "sequences.fasta.gz").string();
//
//    // DNA.
//    {
//        options.sequence_type = sequence_string_set::SequenceType::DNA;
//        sequence_string_set::mock(dir, 3, options);
//        test_validate(dir);
//
//        quick_gzip_write(spath,
//            ">0\nacgt\n"
//            ">1\na.c-g.tacgryswkmbdhvn.-\n"
//            ">2\nTACGRYSWKMBDHVN.-\n"
//        );
//        test_validate(dir);
//
//        quick_gzip_write(spath,
//            ">0\nacgt\n"
//            ">1\nuuuu\n"
//            ">2\nACGT\n"
//        );
//        expect_error("forbidden character 'u'");
//    }
//
//    // FASTQ, for DNA.
//    {
//        options.quality_type = sequence_string_set::QualityType::PHRED33;
//        sequence_string_set::mock(dir, 1, options);
//
//        quick_gzip_write((dir / "sequences.fastq.gz").string(), "@0\nacgu\n+\n;;;;\n");
//        expect_error("forbidden character 'u'");
//
//        options.quality_type = sequence_string_set::QualityType::NONE;
//    }
//
//    // RNA.
//    {
//        options.sequence_type = sequence_string_set::SequenceType::RNA;
//        sequence_string_set::mock(dir, 3, options);
//        test_validate(dir);
//
//        quick_gzip_write(spath,
//            ">0\nacgU\n"
//            ">1\na.c-g.uacgryswkmbdhvn.-\n"
//            ">2\nUACGRYSWKMBDHVN.-\n"
//        );
//        test_validate(dir);
//
//        quick_gzip_write(spath,
//            ">0\nacgu\n"
//            ">1\nTTTT\n"
//            ">2\nACGU\n"
//        );
//        expect_error("forbidden character 'T'");
//    }
//
//    // Protein.
//    {
//        options.sequence_type = sequence_string_set::SequenceType::AA;
//        sequence_string_set::mock(dir, 3, options);
//        test_validate(dir);
//
//        quick_gzip_write(spath,
//            ">0\nacgt\n"
//            ">1\nACD.EFGHIKLMNP-QRSTVWY\n"
//            ">2\nacd.efghiklmnp-qrstvwy\n"
//        );
//        test_validate(dir);
//
//        quick_gzip_write(spath,
//            ">0\nacgt\n"
//            ">1\nxxxx\n"
//            ">2\nACGU\n"
//        );
//        expect_error("forbidden character 'x'");
//    }
//
//    // Custom.
//    {
//        options.sequence_type = sequence_string_set::SequenceType::CUSTOM;
//        sequence_string_set::mock(dir, 3, options);
//        test_validate(dir);
//
//        quick_gzip_write(spath,
//            ">0\nxxxx\n"
//            ">1\n!;;;*8acac~\n"
//            ">2\nacd.efghiklmnp-qrstvwy\n"
//        );
//        test_validate(dir);
//    }
//}
//
//TEST_F(SequenceStringSetTest, QualityType) {
//    sequence_string_set::Options options;
//    auto spath = (dir / "sequences.fastq.gz").string();
//
//    // Phred+33.
//    {
//        options.quality_type = sequence_string_set::QualityType::PHRED33;
//        sequence_string_set::mock(dir, 3, options);
//        test_validate(dir);
//
//        quick_gzip_write(spath,
//            "@0\nacgt\n+\n!!!!\n"
//            "@1\nacgt\n+\n!!!!\n"
//            "@2\nacgt\n+\n!!!!\n"
//        );
//        test_validate(dir);
//
//        quick_gzip_write(spath,
//            "@0\nacgt\n+\n\1\1\1\1\n"
//            "@1\nacgt\n+\n!!!!\n"
//            "@2\nacgt\n+\n!!!!\n"
//        );
//        expect_error("out-of-range quality score");
//    }
//
//    // Phred+64.
//    {
//        options.quality_type = sequence_string_set::QualityType::ILLUMINA64;
//        sequence_string_set::mock(dir, 3, options);
//        test_validate(dir);
//
//        quick_gzip_write(spath,
//            "@0\nacgt\n+\n@@@@\n"
//            "@1\nacgt\n+\n@@@@\n"
//            "@2\nacgt\n+\n@@@@\n"
//        );
//        test_validate(dir);
//
//        quick_gzip_write(spath,
//            "@0\nacgt\n+\n????\n"
//            "@1\nacgt\n+\n@@@@\n"
//            "@2\nacgt\n+\n@@@@\n"
//        );
//        expect_error("out-of-range quality score");
//    }
//
//    // Solexa+64.
//    {
//        options.quality_type = sequence_string_set::QualityType::SOLEXA;
//        sequence_string_set::mock(dir, 3, options);
//        test_validate(dir);
//
//        quick_gzip_write(spath,
//            "@0\nacgt\n+\n;;;;\n"
//            "@1\nacgt\n+\n@@@@\n"
//            "@2\nacgt\n+\n@@@@\n"
//        );
//        test_validate(dir);
//
//        quick_gzip_write(spath,
//            "@0\nacgt\n+\n::::\n"
//            "@1\nacgt\n+\n@@@@\n"
//            "@2\nacgt\n+\n@@@@\n"
//        );
//        expect_error("out-of-range quality score");
//    }
//
//    // None.
//    {
//        options.quality_type = sequence_string_set::QualityType::NONE;
//        sequence_string_set::mock(dir, 10, options);
//        quick_text_write((dir / "OBJECT").string(),
//            "{ \"type\": \"sequence_string_set\", \"sequence_string_set\": { \"version\": \"1.0\", \"length\": 10, \"sequence_type\": \"RNA\", \"quality_type\": \"none\" } }"
//        );
//        test_validate(dir);
//    }
//}
//
//TEST_F(SequenceStringSetTest, Names) {
//    sequence_string_set::Options options;
//    options.quality_type = sequence_string_set::QualityType::PHRED33;
//    sequence_string_set::mock(dir, 10, options);
//
//    auto npath = dir / "names.txt.gz";
//    {
//        byteme::GzipFileWriter writer(npath.c_str(), {});
//        for (size_t i = 0; i < 10; ++i) {
//            std::string tmpname = "\"gene_" + std::to_string(i + 1) + "\"\n";
//            writer.write(reinterpret_cast<const unsigned char*>(tmpname.c_str()), tmpname.size());
//        }
//    }
//    test_validate(dir);
//
//    // Non-parallelized.
//    {
//        takane::Options inopt;
//        inopt.parallel_reads = false;
//        auto meta = takane::read_object_metadata(dir);
//        takane::validate(dir, meta, inopt); 
//    }
//
//    // Works with newlines.
//    {
//        byteme::GzipFileWriter writer(npath.c_str(), {});
//        for (size_t i = 0; i < 10; ++i) {
//            std::string tmpname = "\"gene\n" + std::to_string(i + 1) + "\"\n";
//            writer.write(reinterpret_cast<const unsigned char*>(tmpname.c_str()), tmpname.size());
//        }
//    }
//    test_validate(dir);
//
//    // Errors out with differences in counts.
//    {
//        byteme::GzipFileWriter writer(npath.c_str(), {});
//        for (size_t i = 0; i < 5; ++i) {
//            std::string tmpname = "\"gene-" + std::to_string(i + 1) + "\"\n";
//            writer.write(reinterpret_cast<const unsigned char*>(tmpname.c_str()), tmpname.size());
//        }
//    }
//    expect_error("number of names");
//
//    // Errors out with quotes.
//    {
//        byteme::GzipFileWriter writer(npath.c_str(), {});
//        for (size_t i = 0; i < 10; ++i) {
//            std::string tmpname = "gene-" + std::to_string(i + 1) + "\n";
//            writer.write(reinterpret_cast<const unsigned char*>(tmpname.c_str()), tmpname.size());
//        }
//    }
//    expect_error("should start with a quote");
//
//    {
//        byteme::GzipFileWriter writer(npath.c_str(), {});
//        for (size_t i = 0; i < 10; ++i) {
//            std::string tmpname = "\"gene-" + std::to_string(i + 1) + "\"a\n";
//            writer.write(reinterpret_cast<const unsigned char*>(tmpname.c_str()), tmpname.size());
//        }
//    }
//    expect_error("present after end quote");
//
//    // Errors out if there no terminating newline.
//    {
//        byteme::GzipFileWriter writer(npath.c_str(), {});
//        for (size_t i = 0; i < 10; ++i) {
//            std::string tmpname = "\"gene-" + std::to_string(i + 1) + "\"";
//            if (i < 9) {
//                tmpname += "\n";
//            }
//            writer.write(reinterpret_cast<const unsigned char*>(tmpname.c_str()), tmpname.size());
//        }
//    }
//    expect_error("premature end");
//}
//
//TEST_F(SequenceStringSetTest, Metadata) {
//    sequence_string_set::Options options;
//    sequence_string_set::mock(dir, 20, options);
//
//    auto odir = dir / "other_annotations";
//    auto rdir = dir / "sequence_annotations";
//
//    initialize_directory_simple(rdir, "simple_list", "1.0");
//    expect_error("'DATA_FRAME'"); 
//
//    data_frame::mock(rdir, 20, {});
//    initialize_directory_simple(odir, "data_frame", "1.0");
//    expect_error("'SIMPLE_LIST'");
//
//    simple_list::mock(odir);
//    test_validate(dir);
//}
