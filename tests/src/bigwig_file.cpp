#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "takane/bigwig_file.hpp"
#include "utils.h"

#include <string>
#include <vector>
#include <filesystem>
#include <stdexcept>

TEST(BigWigFile, Okay) {
    auto dir = define_test_path("bigwig_file");
    auto bwpath = (dir / "file.bw").string();

    {
        initialize_directory_simple(dir, "bigwig_file", "1.0");
        byteme::RawFileWriter handle(bwpath.c_str(), {});
        uint32_t val = 0x888FFC26;
        handle.write(reinterpret_cast<unsigned char*>(&val), sizeof(val));
    }
    test_validate(dir);

    {
        initialize_directory_simple(dir, "bigwig_file", "1.0");
        byteme::RawFileWriter handle(bwpath.c_str(), {});
        uint32_t val = 0x26FC8F88;
        handle.write(reinterpret_cast<unsigned char*>(&val), sizeof(val));
    }
    test_validate(dir);

    // Handles strict validation.
    {
        takane::Options opts;
        opts.bigwig_file_strict_check = [](const std::filesystem::path&, const takane::ObjectMetadata&, const takane::Options&) {};
        test_validate(dir, opts);
    }
}

TEST(BigWigFile, VersionError) {
    auto dir = define_test_path("bigwig_file");

    {
        initialize_directory_simple(dir, "bigwig_file", "2.0");
    }
    expect_validation_error(dir, "unsupported version");
}

TEST(BigWigFile, BigWigError) {
    auto dir = define_test_path("bigwig_file");
    auto bwpath = (dir / "file.bw").string();

    {
        initialize_directory_simple(dir, "bigwig_file", "1.0");
        quick_text_write(bwpath, "foobar");
    }
    expect_validation_error(dir, "incorrect signature for a bigWig file");
}

TEST(BigWigFile, StrictError) {
    auto dir = define_test_path("bigwig_file");
    auto bwpath = (dir / "file.bw").string();

    {
        initialize_directory_simple(dir, "bigwig_file", "1.0");
        byteme::RawFileWriter handle(bwpath.c_str(), {});
        uint32_t val = 0x888FFC26;
        handle.write(reinterpret_cast<unsigned char*>(&val), sizeof(val));
    }

    takane::Options opts;
    opts.bigwig_file_strict_check = [](const std::filesystem::path&, const takane::ObjectMetadata&, const takane::Options&) { throw std::runtime_error("ARGH"); };
    expect_validation_error(dir, "ARGH", opts);
}
