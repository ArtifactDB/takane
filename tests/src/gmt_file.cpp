#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "takane/gmt_file.hpp"
#include "utils.h"

#include <string>
#include <vector>
#include <filesystem>
#include <stdexcept>

TEST(GmtFile, Okay) {
    auto dir = define_test_path("gmt_file");

    {
        initialize_directory_simple(dir, "gmt_file", "1.0");
        quick_gzip_write(dir / "file.gmt.gz", "set\tmy set\ta\tb\tc\n");
    }
    test_validate(dir);

    // Checking that the strict validation runs.
    {
        takane::Options opts;
        opts.gmt_file_strict_check = [](const std::filesystem::path&, const takane::ObjectMetadata&, const takane::Options&) {};
        test_validate(dir);
    }
}

TEST(GmtFile, VersionError) {
    auto dir = define_test_path("gmt_file");

    {
        initialize_directory_simple(dir, "gmt_file", "2.0");
    }
    expect_validation_error(dir, "unsupported version");
}

TEST(GmtFile, GmtError) {
    auto dir = define_test_path("gmt_file");

    {
        initialize_directory_simple(dir, "gmt_file", "1.0");
        quick_text_write(dir / "file.gmt.gz", "WHEE");
    }
    expect_validation_error(dir, "GZIP file");
}

TEST(GmtFile, StrictError) {
    auto dir = define_test_path("gmt_file");

    {
        initialize_directory_simple(dir, "gmt_file", "1.0");
        quick_gzip_write(dir / "file.gmt.gz", "set\tmy set\ta\tb\tc\n");
    }

    takane::Options opts;
    opts.gmt_file_strict_check = [](const std::filesystem::path&, const takane::ObjectMetadata&, const takane::Options&) { throw std::runtime_error("ARGH"); };
    expect_validation_error(dir, "ARGH", opts);
}
