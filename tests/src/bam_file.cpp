#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "takane/bam_file.hpp"
#include "utils.h"

#include <string>
#include <vector>
#include <filesystem>
#include <stdexcept>

TEST(BamFile, Okay) {
    auto dir = define_test_path("bam_file");

    {
        initialize_directory_simple(dir, "bam_file", "1.0");
        quick_gzip_write(dir / "file.bam", "BAM\1");
    }
    test_validate(dir);

    // Handles strict validation.
    {
        takane::Options opts;
        opts.bam_file_strict_check = [](const std::filesystem::path&, const takane::ObjectMetadata&, const takane::Options&) {};
        test_validate(dir, opts);
    }

    // With one or both indices.
    {
        initialize_directory_simple(dir, "bam_file", "1.0");
        quick_gzip_write(dir / "file.bam", "BAM\1");
        quick_text_write(dir / "file.bam.bai", "BAI\1");
    }
    test_validate(dir);

    {
        initialize_directory_simple(dir, "bam_file", "1.0");
        quick_gzip_write(dir / "file.bam", "BAM\1");
        quick_text_write(dir / "file.bam.bai", "BAI\1");
        quick_gzip_write(dir / "file.bam.csi", "CSI\1");
    }
    test_validate(dir);
}

TEST(BamFile, VersionError) {
    auto dir = define_test_path("bam_file");

    {
        initialize_directory_simple(dir, "bam_file", "2.0");
    }
    expect_validation_error(dir, "unsupported version");
}

TEST(BamFile, BamError) {
    auto dir = define_test_path("bam_file");

    {
        initialize_directory_simple(dir, "bam_file", "1.0");
        quick_text_write(dir / "file.bam", "FOO");
    }
    expect_validation_error(dir, "incorrect signature for a GZIP file");

    {
        initialize_directory_simple(dir, "bam_file", "1.0");
        quick_gzip_write(dir / "file.bam", "foo\1");
    }
    expect_validation_error(dir, "incorrect signature for a BAM file");
}

TEST(BamFile, IndexError) {
    auto dir = define_test_path("bam_file");

    {
        initialize_directory_simple(dir, "bam_file", "1.0");
        quick_gzip_write(dir / "file.bam", "BAM\1");
        quick_text_write(dir / "file.bam.bai", "foobar\1");
    }
    expect_validation_error(dir, "incorrect signature for a BAM index");

    {
        initialize_directory_simple(dir, "bam_file", "1.0");
        quick_gzip_write(dir / "file.bam", "BAM\1");
        quick_text_write(dir / "file.bam.csi", "FOO");
    }
    expect_validation_error(dir, "incorrect signature for a GZIP file");

    {
        initialize_directory_simple(dir, "bam_file", "1.0");
        quick_gzip_write(dir / "file.bam", "BAM\1");
        quick_gzip_write(dir / "file.bam.csi", "FOOBAR");
    }
    expect_validation_error(dir, "incorrect signature for a CSI file");
}

TEST(BamFile, StrictError) {
    auto dir = define_test_path("bam_file");

    {
        initialize_directory_simple(dir, "bam_file", "1.0");
        quick_gzip_write(dir / "file.bam", "BAM\1");
    }

    takane::Options opts;
    opts.bam_file_strict_check = [](const std::filesystem::path&, const takane::ObjectMetadata&, const takane::Options&) { throw std::runtime_error("ARGH"); };
    expect_validation_error(dir, "ARGH", opts);
}
