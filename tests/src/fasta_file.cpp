#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "takane/fasta_file.hpp"
#include "utils.h"

#include <string>
#include <vector>
#include <filesystem>
#include <stdexcept>

TEST(FastaFile, Okay) {
    auto dir = define_test_path("fasta_file");

    {
        initialize_directory(dir);
        quick_text_write(
            dir / "OBJECT",
            "{ \"type\": \"fasta_file\", \"fasta_file\": { \"version\": \"1.0\", \"sequence_type\": \"DNA\" } }"
        );
        quick_gzip_write(dir / "file.fasta.gz", ">asdasd\nACGT\n");
    }
    test_validate(dir);

    {
        initialize_directory(dir);
        quick_text_write(
            dir / "OBJECT",
            "{ \"type\": \"fasta_file\", \"fasta_file\": { \"version\": \"1.0\", \"indexed\": true, \"sequence_type\": \"DNA\" } }"
        );
        quick_gzip_write(dir / "file.fasta.bgz", ">asdasd\nACGT\n");
        quick_text_write(dir / "file.fasta.fai", "");
        quick_text_write(dir / "file.fasta.bgz.gzi", "");
    }
    test_validate(dir);

    // Works with different sequence types.
    {
        initialize_directory(dir);
        quick_text_write(
            dir / "OBJECT",
            "{ \"type\": \"fasta_file\", \"fasta_file\": { \"version\": \"1.0\", \"sequence_type\": \"RNA\" } }"
        );
        quick_gzip_write(dir / "file.fasta.gz", ">asdasd\nACGT\n");
    }
    test_validate(dir);

    {
        initialize_directory(dir);
        quick_text_write(
            dir / "OBJECT",
            "{ \"type\": \"fasta_file\", \"fasta_file\": { \"version\": \"1.0\", \"sequence_type\": \"AA\" } }"
        );
        quick_gzip_write(dir / "file.fasta.gz", ">asdasd\nACGT\n");
    }
    test_validate(dir);

    {
        initialize_directory(dir);
        quick_text_write(
            dir / "OBJECT",
            "{ \"type\": \"fasta_file\", \"fasta_file\": { \"version\": \"1.0\", \"sequence_type\": \"custom\" } }"
        );
        quick_gzip_write(dir / "file.fasta.gz", ">asdasd\nACGT\n");
    }
    test_validate(dir);

    // Works with the strict validator.
    {
        takane::Options opts;
        opts.fasta_file_strict_check = [](const std::filesystem::path&, const takane::ObjectMetadata&, const takane::Options&, bool) {};
        test_validate(dir, opts);
    }
}

TEST(FastaFile, VersionError) {
    auto dir = define_test_path("fasta_file");

    {
        initialize_directory_simple(dir, "fasta_file", "2.0");
    }
    expect_validation_error(dir, "unsupported version");
}

TEST(FastaFile, SequenceTypeError) {
    auto dir = define_test_path("fasta_file");

    {
        initialize_directory(dir);
        quick_text_write(
            dir / "OBJECT",
            "{ \"type\": \"fasta_file\", \"fasta_file\": { \"version\": \"1.0\", \"sequence_type\": \"foo\" } }"
        );
        quick_gzip_write(dir / "file.fasta.gz", "asdasd\nACGT\n");
    }
    expect_validation_error(dir, "unsupported value 'foo'");
}

TEST(FastaFile, FastaError) {
    auto dir = define_test_path("fasta_file");

    {
        initialize_directory(dir);
        quick_text_write(
            dir / "OBJECT",
            "{ \"type\": \"fasta_file\", \"fasta_file\": { \"version\": \"1.0\", \"sequence_type\": \"RNA\" } }"
        );
        quick_gzip_write(dir / "file.fasta.gz", "asdasd\nACGT\n");
    }
    expect_validation_error(dir, "start with '>'");

    // Also validate the indexed version.
    {
        initialize_directory(dir);
        quick_text_write(
            dir / "OBJECT",
            "{ \"type\": \"fasta_file\", \"fasta_file\": { \"version\": \"1.0\", \"sequence_type\": \"AA\", \"indexed\": true } }"
        );
        quick_gzip_write(dir / "file.fasta.bgz", "asdasd\nACGT\n");
    }
    expect_validation_error(dir, "start with '>'");
}

TEST(FastaFile, IndexedError) {
    auto dir = define_test_path("fasta_file");

    {
        initialize_directory(dir);
        quick_text_write(
            dir / "OBJECT",
            "{ \"type\": \"fasta_file\", \"fasta_file\": { \"version\": \"1.0\", \"indexed\": null } }"
        );
        quick_gzip_write(dir / "file.fasta.gz", "asdasd\nACGT\n");
    }
    expect_validation_error(dir, "should be a JSON boolean");

    {
        initialize_directory(dir);
        quick_text_write(
            dir / "OBJECT",
            "{ \"type\": \"fasta_file\", \"fasta_file\": { \"version\": \"1.0\", \"indexed\": true, \"sequence_type\": \"DNA\" } }"
        );
        quick_gzip_write(dir / "file.fasta.bgz", ">asdasd\nACGT\n");
    }
    expect_validation_error(dir, "missing FASTA index file");

    {
        quick_text_write(dir / "file.fasta.fai", "");
    }
    expect_validation_error(dir, "missing BGZF index file");
}

TEST(FastaFile, Strict) {
    auto dir = define_test_path("fasta_file");

    {
        initialize_directory(dir);
        quick_text_write(
            dir / "OBJECT",
            "{ \"type\": \"fasta_file\", \"fasta_file\": { \"version\": \"1.0\", \"sequence_type\": \"DNA\" } }"
        );
        quick_gzip_write(dir / "file.fasta.gz", ">asdasd\nACGT\n");
    }

    takane::Options opts;
    opts.fasta_file_strict_check = [](const std::filesystem::path&, const takane::ObjectMetadata&, const takane::Options&, bool) { throw std::runtime_error("ARGH"); };
    expect_validation_error(dir, "ARGH", opts);
}
