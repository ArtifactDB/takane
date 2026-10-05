#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "takane/fastq_file.hpp"
#include "utils.h"

#include <string>
#include <vector>
#include <filesystem>
#include <stdexcept>

TEST(FastqFile, Okay) {
    auto dir = define_test_path("fastq_file");

    {
        initialize_directory(dir);
        quick_text_write(
            dir / "OBJECT",
            "{ \"type\": \"fastq_file\", \"fastq_file\": { \"version\": \"1.0\", \"sequence_type\": \"DNA\", \"quality_type\": \"phred\", \"quality_offset\": 33 } }"
        );
        quick_gzip_write(dir / "file.fastq.gz", "@asdasd\nACGT\n+\n!!!!\n");
    }
    test_validate(dir);

    {
        initialize_directory(dir);
        quick_text_write(
            dir / "OBJECT",
            "{ \"type\": \"fastq_file\", \"fastq_file\": { \"version\": \"1.0\", \"indexed\": true, \"sequence_type\": \"DNA\", \"quality_type\": \"solexa\" } }"
        );
        quick_gzip_write(dir / "file.fastq.bgz", "@asdasd\nACGT\n+\n!!!!\n");
        quick_text_write(dir / "file.fastq.fai", "");
        quick_text_write(dir / "file.fastq.bgz.gzi", "");
    }
    test_validate(dir);

    // Works with Phred+64.
    {
        initialize_directory(dir);
        quick_text_write(
            dir / "OBJECT",
            "{ \"type\": \"fastq_file\", \"fastq_file\": { \"version\": \"1.0\", \"sequence_type\": \"DNA\", \"quality_type\": \"phred\", \"quality_offset\": 64 } }"
        );
        quick_gzip_write(dir / "file.fastq.gz", "@asdasd\nACGT\n+\n!!!!\n");
    }

    // Works with Solexa.
    {
        initialize_directory(dir);
        quick_text_write(
            dir / "OBJECT",
            "{ \"type\": \"fastq_file\", \"fastq_file\": { \"version\": \"1.0\", \"sequence_type\": \"DNA\", \"quality_type\": \"solexa\" } }"
        );
        quick_gzip_write(dir / "file.fastq.gz", "@asdasd\nACGT\n+\n!!!!\n");
    }

    // Works with the strict validator.
    {
        takane::Options opts;
        opts.fastq_file_strict_check = [](const std::filesystem::path&, const takane::ObjectMetadata&, const takane::Options&, bool) {};
        test_validate(dir, opts);
    }
}

TEST(FastqFile, VersionError) {
    auto dir = define_test_path("fastq_file");

    {
        initialize_directory_simple(dir, "fastq_file", "2.0");
    }
    expect_validation_error(dir, "unsupported version");
}

TEST(FastqFile, SequenceTypeError) {
    auto dir = define_test_path("fastq_file");

    {
        initialize_directory(dir);
        quick_text_write(
            dir / "OBJECT",
            "{ \"type\": \"fastq_file\", \"fastq_file\": { \"version\": \"1.0\" } }"
        );
        quick_gzip_write(dir / "file.fastq.gz", "@asdasd\nACGT\n+\n!!!!\n");
    }
    expect_validation_error(dir, "sequence_type");
}

TEST(FastqFile, QualityError) {
    auto dir = define_test_path("fastq_file");

    // Running through our checks for 'quality_type'.
    {
        initialize_directory(dir);
        quick_text_write(
            dir / "OBJECT",
            "{ \"type\": \"fastq_file\", \"fastq_file\": { \"version\": \"1.0\", \"sequence_type\": \"DNA\" } }"
        );
        quick_gzip_write(dir / "file.fastq.gz", "@asdasd\nACGT\n+\n!!!!\n");
    }
    expect_validation_error(dir, "not present");

    {
        quick_text_write(
            dir / "OBJECT",
            "{ \"type\": \"fastq_file\", \"fastq_file\": { \"version\": \"1.0\", \"sequence_type\": \"DNA\", \"quality_type\": true } }"
        );
    }
    expect_validation_error(dir, "JSON string");

    {
        quick_text_write(
            dir / "OBJECT",
            "{ \"type\": \"fastq_file\", \"fastq_file\": { \"version\": \"1.0\", \"sequence_type\": \"DNA\", \"quality_type\": \"foo\" } }"
        );
    }
    expect_validation_error(dir, "unknown value 'foo'");

    // Checking the quality offset.
    {
        quick_text_write(
            dir / "OBJECT",
            "{ \"type\": \"fastq_file\", \"fastq_file\": { \"version\": \"1.0\", \"sequence_type\": \"DNA\", \"quality_type\": \"phred\" } }"
        );
    }
    expect_validation_error(dir, "not present");

    {
        quick_text_write(
            dir / "OBJECT",
            "{ \"type\": \"fastq_file\", \"fastq_file\": { \"version\": \"1.0\", \"sequence_type\": \"DNA\", \"quality_type\": \"phred\", \"quality_offset\": true } }"
        );
    }
    expect_validation_error(dir, "JSON number");

    {
        quick_text_write(
            dir / "OBJECT",
            "{ \"type\": \"fastq_file\", \"fastq_file\": { \"version\": \"1.0\", \"sequence_type\": \"DNA\", \"quality_type\": \"phred\", \"quality_offset\": 20 } }"
        );
    }
    expect_validation_error(dir, "33 or 64");
}

TEST(FastqFile, FastqError) {
    auto dir = define_test_path("fastq_file");

    {
        initialize_directory(dir);
        quick_text_write(
            dir / "OBJECT",
            "{ \"type\": \"fastq_file\", \"fastq_file\": { \"version\": \"1.0\", \"sequence_type\": \"DNA\", \"quality_type\": \"solexa\" } }"
        );
        quick_gzip_write(dir / "file.fastq.gz", "asdasd\nACGT\n+\n!!!!\n");
    }
    expect_validation_error(dir, "does not start with '@'");
}

TEST(FastqFile, IndexedError) {
    auto dir = define_test_path("fastq_file");

    {
        initialize_directory(dir);
        quick_text_write(
            dir / "OBJECT",
            "{ \"type\": \"fastq_file\", \"fastq_file\": { \"version\": \"1.0\", \"indexed\": true, \"sequence_type\": \"DNA\", \"quality_type\": \"solexa\" } }"
        );
        quick_gzip_write(dir / "file.fastq.bgz", "@asdasd\nACGT\n+\n!!!!\n");
    }
    expect_validation_error(dir, "missing FASTQ index file");

    {
        quick_gzip_write(dir / "file.fastq.fai", "");
    }
    expect_validation_error(dir, "missing BGZF index file");
}

TEST(FastqFile, StrictError) {
    auto dir = define_test_path("fastq_file");

    {
        initialize_directory(dir);
        quick_text_write(
            dir / "OBJECT",
            "{ \"type\": \"fastq_file\", \"fastq_file\": { \"version\": \"1.0\", \"sequence_type\": \"DNA\", \"quality_type\": \"phred\", \"quality_offset\": 64 } }"
        );
        quick_gzip_write(dir / "file.fastq.gz", "@asdasd\nACGT\n+\n!!!!\n");
    }

    takane::Options opts;
    opts.fastq_file_strict_check = [](const std::filesystem::path&, const takane::ObjectMetadata&, const takane::Options&, bool) { throw std::runtime_error("ARGH"); };
    expect_validation_error(dir, "ARGH", opts);
}
