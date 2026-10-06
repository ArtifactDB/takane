#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "takane/gff_file.hpp"
#include "utils.h"

#include <string>
#include <vector>
#include <filesystem>
#include <stdexcept>

TEST(GffFile, Okay) {
    auto dir = define_test_path("gff_file");

    {
        initialize_directory(dir);
        quick_text_write(
            dir / "OBJECT",
            "{ \"type\": \"gff_file\", \"gff_file\": { \"version\": \"1.0\", \"format\": \"GFF2\" } }"
        );
        quick_gzip_write(dir / "file.gff2.gz", "chr1\t1\t2\n");
    }
    test_validate(dir);

    // Works with GFF3.
    {
        initialize_directory(dir);
        quick_text_write(
            dir / "OBJECT",
            "{ \"type\": \"gff_file\", \"gff_file\": { \"version\": \"1.0\", \"format\": \"GFF3\" } }"
        );
        quick_gzip_write(dir / "file.gff3.gz", "##gff-version 3.1.26\nchr1\t1\t2\n");
    }
    test_validate(dir);

    // Works with indexing.
    {
        initialize_directory(dir);
        quick_text_write(
            dir / "OBJECT",
            "{ \"type\": \"gff_file\", \"gff_file\": { \"version\": \"1.0\", \"format\": \"GFF2\", \"indexed\": true } }"
        );
        quick_gzip_write(dir / "file.gff2.bgz", "##chr1\t1\t2\n");
        quick_gzip_write(dir / "file.gff2.bgz.tbi", "TBI\1");
    }
    test_validate(dir);

    // Handles the strict validator.
    {
        takane::Options opts;
        opts.gff_file_strict_check = [](const std::filesystem::path&, const takane::ObjectMetadata&, const takane::Options&, bool) {};
        test_validate(dir);
    }
}

TEST(GffFile, VersionError) {
    auto dir = define_test_path("gff_file");

    {
        initialize_directory_simple(dir, "gff_file", "2.0");
    }
    expect_validation_error(dir, "unsupported version");
}

TEST(GffFile, FormatError) {
    auto dir = define_test_path("gff_file");

    {
        initialize_directory(dir);
        quick_text_write(
            dir / "OBJECT",
            "{ \"type\": \"gff_file\", \"gff_file\": { \"version\": \"1.0\", \"format\": true } }"
        );
    }
    expect_validation_error(dir, "JSON string");

    {
        initialize_directory(dir);
        quick_text_write(
            dir / "OBJECT",
            "{ \"type\": \"gff_file\", \"gff_file\": { \"version\": \"1.0\", \"format\": \"FOO\" } }"
        );
    }
    expect_validation_error(dir, "unknown value");
}

TEST(GffFile, GffError) {
    auto dir = define_test_path("gff_file");

    {
        initialize_directory(dir);
        quick_text_write(
            dir / "OBJECT",
            "{ \"type\": \"gff_file\", \"gff_file\": { \"version\": \"1.0\", \"format\": \"GFF2\" } }"
        );
        quick_text_write(dir / "file.gff2.gz", "chr1\t1\t2\n");
    }
    expect_validation_error(dir, "GZIP file");


    {
        initialize_directory(dir);
        quick_text_write(
            dir / "OBJECT",
            "{ \"type\": \"gff_file\", \"gff_file\": { \"version\": \"1.0\", \"format\": \"GFF3\" } }"
        );
        quick_gzip_write(dir / "file.gff3.gz", "chr1\t1\t2\n");
    }
    expect_validation_error(dir, "GFF3 file");
}

TEST(GffFile, IndexedError) {
    auto dir = define_test_path("gff_file");

    {
        initialize_directory(dir);
        quick_text_write(
            dir / "OBJECT",
            "{ \"type\": \"gff_file\", \"gff_file\": { \"version\": \"1.0\", \"format\": \"GFF2\", \"indexed\": true } }"
        );
        quick_gzip_write(dir / "file.gff2.bgz", "chr1\t1\t2\n");
        quick_text_write(dir / "file.gff2.bgz.tbi", "");
    }
    expect_validation_error(dir, "GZIP file");

    {
        initialize_directory(dir);
        quick_text_write(
            dir / "OBJECT",
            "{ \"type\": \"gff_file\", \"gff_file\": { \"version\": \"1.0\", \"format\": \"GFF2\", \"indexed\": true } }"
        );
        quick_gzip_write(dir / "file.gff2.bgz", "chr1\t1\t2\n");
        quick_gzip_write(dir / "file.gff2.bgz.tbi", "Foobar");
    }
    expect_validation_error(dir, "tabix file");
}

TEST(GffFile, StrictError) {
    auto dir = define_test_path("gff_file");

    {
        initialize_directory(dir);
        quick_text_write(
            dir / "OBJECT",
            "{ \"type\": \"gff_file\", \"gff_file\": { \"version\": \"1.0\", \"format\": \"GFF2\" } }"
        );
        quick_gzip_write(dir / "file.gff2.gz", "chr1\t1\t2\n");
    }

    takane::Options opts;
    opts.gff_file_strict_check = [](const std::filesystem::path&, const takane::ObjectMetadata&, const takane::Options&, bool) { throw std::runtime_error("ARGH"); };
    expect_validation_error(dir, "ARGH", opts);
}
