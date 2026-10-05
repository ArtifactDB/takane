#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "takane/bed_file.hpp"
#include "utils.h"

#include <string>
#include <vector>
#include <filesystem>
#include <stdexcept>

TEST(BedFile, Okay) {
    auto dir = define_test_path("bed_file");

    {
        initialize_directory(dir);
        quick_text_write(
            dir / "OBJECT",
            "{ \"type\": \"bed_file\", \"bed_file\": { \"version\": \"1.0\" } }"
        );
        quick_gzip_write(dir / "file.bed.gz", "chr1\t1\t2\n");
    }
    test_validate(dir);

    {
        initialize_directory(dir);
        quick_text_write(
            dir / "OBJECT",
            "{ \"type\": \"bed_file\", \"bed_file\": { \"version\": \"1.0\", \"indexed\": true } }"
        );
        quick_gzip_write(dir / "file.bed.bgz", "chr1\t1\t2\n");
        quick_gzip_write(dir / "file.bed.bgz.tbi", "TBI\1");
    }
    test_validate(dir);

    // Handles strict validation.
    {
        takane::Options opts;
        opts.bed_file_strict_check = [](const std::filesystem::path&, const takane::ObjectMetadata&, const takane::Options&, bool) {};
        test_validate(dir);
    }
}

TEST(BedFile, VersionError) {
    auto dir = define_test_path("bed_file");

    {
        initialize_directory_simple(dir, "bed_file", "2.0");
    }
    expect_validation_error(dir, "unsupported version");
}

TEST(BedFile, BedError) {
    auto dir = define_test_path("bed_file");

    {
        initialize_directory(dir);
        quick_text_write(
            dir / "OBJECT",
            "{ \"type\": \"bed_file\", \"bed_file\": { \"version\": \"1.0\" } }"
        );
        quick_text_write(dir / "file.bed.gz", "WHEE");
    }
    expect_validation_error(dir, "GZIP file");
}

TEST(BedFile, IndexedError) {
    auto dir = define_test_path("bed_file");

    {
        initialize_directory(dir);
        quick_text_write(
            dir / "OBJECT",
            "{ \"type\": \"bed_file\", \"bed_file\": { \"version\": \"1.0\", \"indexed\": true } }"
        );
        quick_gzip_write(dir / "file.bed.bgz", "chr1\t1\t2\n");
        quick_gzip_write(dir / "file.bed.bgz.tbi", "YAY");
    }
    expect_validation_error(dir, "tabix file");
}

TEST(BedFile, StrictError) {
    auto dir = define_test_path("bed_file");

    {
        initialize_directory(dir);
        quick_text_write(
            dir / "OBJECT",
            "{ \"type\": \"bed_file\", \"bed_file\": { \"version\": \"1.0\", \"indexed\": false } }"
        );
        quick_gzip_write(dir / "file.bed.gz", "chr1\t1\t2\n");
    }

    takane::Options opts;
    opts.bed_file_strict_check = [](const std::filesystem::path&, const takane::ObjectMetadata&, const takane::Options&, bool) { throw std::runtime_error("ARGH"); };
    expect_validation_error(dir, "ARGH", opts);
}
