#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "takane/bcf_file.hpp"
#include "utils.h"

#include <string>
#include <vector>
#include <filesystem>
#include <stdexcept>

TEST(BcfFile, Okay) {
    auto dir = define_test_path("bcf_file");

    {
        initialize_directory_simple(dir, "bcf_file", "1.0");
        quick_gzip_write(dir / "file.bcf", "BCF\2\1");
    }
    test_validate(dir);

    // Handles strict validation.
    {
        takane::Options opts;
        opts.bcf_file_strict_check = [](const std::filesystem::path&, const takane::ObjectMetadata&, const takane::Options&) {};
        test_validate(dir, opts);
    }

    // Works with the older format.
    {
        initialize_directory_simple(dir, "bcf_file", "1.0");
        quick_gzip_write(dir / "file.bcf", "BCF\4");
    }
    test_validate(dir);

    {
        initialize_directory_simple(dir, "bcf_file", "1.0");
        quick_gzip_write(dir / "file.bcf", "BCF\4asdasdasd"); // throwing in some trailing junk.
    }
    test_validate(dir);

    // With one or both indices.
    {
        initialize_directory_simple(dir, "bcf_file", "1.0");
        quick_gzip_write(dir / "file.bcf", "BCF\2\1");
        quick_gzip_write(dir / "file.bcf.tbi", "TBI\1");
    }
    test_validate(dir);

    {
        initialize_directory_simple(dir, "bcf_file", "1.0");
        quick_gzip_write(dir / "file.bcf", "BCF\2\1");
        quick_gzip_write(dir / "file.bcf.tbi", "TBI\1");
        quick_gzip_write(dir / "file.bcf.csi", "CSI\1");
    }
    test_validate(dir);
}

TEST(BcfFile, VersionError) {
    auto dir = define_test_path("bcf_file");

    {
        initialize_directory_simple(dir, "bcf_file", "2.0");
    }
    expect_validation_error(dir, "unsupported version");
}

TEST(BcfFile, BcfError) {
    auto dir = define_test_path("bcf_file");

    {
        initialize_directory_simple(dir, "bcf_file", "1.0");
        quick_text_write(dir / "file.bcf", "foo\1");
    }
    expect_validation_error(dir, "incorrect signature for a GZIP file");

    {
        initialize_directory_simple(dir, "bcf_file", "1.0");
        quick_gzip_write(dir / "file.bcf", "foo");
    }
    expect_validation_error(dir, "file is too short");

    {
        initialize_directory_simple(dir, "bcf_file", "1.0");
        quick_gzip_write(dir / "file.bcf", "foobar\2\1");
    }
    expect_validation_error(dir, "incorrect signature for a BCF file");
}

TEST(BcfFile, IndexError) {
    auto dir = define_test_path("bcf_file");

    {
        initialize_directory_simple(dir, "bcf_file", "1.0");
        quick_gzip_write(dir / "file.bcf", "BCF\2\1");
        quick_text_write(dir / "file.bcf.tbi", "foobar\1");
    }
    expect_validation_error(dir, "incorrect signature for a GZIP file");

    {
        initialize_directory_simple(dir, "bcf_file", "1.0");
        quick_gzip_write(dir / "file.bcf", "BCF\2\1");
        quick_gzip_write(dir / "file.bcf.tbi", "foobar\1");
    }
    expect_validation_error(dir, "incorrect signature for a tabix file");

    {
        initialize_directory_simple(dir, "bcf_file", "1.0");
        quick_gzip_write(dir / "file.bcf", "BCF\2\1");
        quick_text_write(dir / "file.bcf.csi", "foobar\1");
    }
    expect_validation_error(dir, "incorrect signature for a GZIP file");

    {
        initialize_directory_simple(dir, "bcf_file", "1.0");
        quick_gzip_write(dir / "file.bcf", "BCF\2\1");
        quick_gzip_write(dir / "file.bcf.csi", "foobar\1");
    }
    expect_validation_error(dir, "incorrect signature for a CSI file");
}

TEST(BcfFile, Strict) {
    auto dir = define_test_path("bcf_file");

    {
        initialize_directory_simple(dir, "bcf_file", "1.0");
        quick_gzip_write(dir / "file.bcf", "BCF\2\1");
    }

    takane::Options opts;
    opts.bcf_file_strict_check = [](const std::filesystem::path&, const takane::ObjectMetadata&, const takane::Options&) { throw std::runtime_error("ARGH"); };
    expect_validation_error(dir, "ARGH", opts);
}
