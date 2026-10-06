#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "takane/rds_file.hpp"
#include "utils.h"

#include <string>
#include <vector>
#include <filesystem>
#include <stdexcept>

TEST(RdsFile, Okay) {
    auto dir = define_test_path("rds_file");

    {
        initialize_directory_simple(dir, "rds_file", "1.0");
        quick_gzip_write(dir / "file.rds", "X\n");
    }
    test_validate(dir);

    // Works with strict validation.
    {
        takane::Options opts;
        opts.rds_file_strict_check = [](const std::filesystem::path&, const takane::ObjectMetadata&, const takane::Options&) {};
        test_validate(dir);
    }
}

TEST(RdsFile, VersionError) {
    auto dir = define_test_path("rds_file");

    {
        initialize_directory_simple(dir, "rds_file", "2.0");
    }
    expect_validation_error(dir, "unsupported version");
}

TEST(RdsFile, RdsError) {
    auto dir = define_test_path("rds_file");

    {
        initialize_directory_simple(dir, "rds_file", "1.0");
        quick_text_write(dir / "file.rds", "X\n");
    }
    expect_validation_error(dir, "GZIP file");

    {
        initialize_directory_simple(dir, "rds_file", "1.0");
        quick_gzip_write(dir / "file.rds", "B\n");
    }
    expect_validation_error(dir, "RDS file");
}

TEST(RdsFile, StrictError) {
    auto dir = define_test_path("rds_file");

    {
        initialize_directory_simple(dir, "rds_file", "1.0");
        quick_gzip_write(dir / "file.rds", "X\n");
    }

    takane::Options opts;
    opts.rds_file_strict_check = [](const std::filesystem::path&, const takane::ObjectMetadata&, const takane::Options&) { throw std::runtime_error("ARGH"); };
    expect_validation_error(dir, "ARGH", opts);
}
