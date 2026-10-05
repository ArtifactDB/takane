#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "takane/bigbed_file.hpp"
#include "utils.h"

#include <string>
#include <vector>
#include <filesystem>
#include <stdexcept>

TEST(BigBedFile, Okay) {
    auto dir = define_test_path("bigbed_file");
    auto bbpath = (dir / "file.bb").string();

    {
        initialize_directory_simple(dir, "bigbed_file", "1.0");
        byteme::RawFileWriter handle(bbpath.c_str(), {});
        uint32_t val = 0x8789F2EB;
        handle.write(reinterpret_cast<unsigned char*>(&val), sizeof(val));
    }
    test_validate(dir);

    {
        initialize_directory_simple(dir, "bigbed_file", "1.0");
        byteme::RawFileWriter handle(bbpath.c_str(), {});
        uint32_t val = 0xEBF28987;
        handle.write(reinterpret_cast<unsigned char*>(&val), sizeof(val));
    }
    test_validate(dir);

    // Handles strict validation.
    {
        takane::Options opts;
        opts.bigbed_file_strict_check = [](const std::filesystem::path&, const takane::ObjectMetadata&, const takane::Options&) {};
        test_validate(dir, opts);
    }
}

TEST(BigBedFile, VersionError) {
    auto dir = define_test_path("bigbed_file");

    {
        initialize_directory_simple(dir, "bigbed_file", "2.0");
    }
    expect_validation_error(dir, "unsupported version");
}

TEST(BigBedFile, BigBedError) {
    auto dir = define_test_path("bigbed_file");
    auto bbpath = (dir / "file.bb").string();

    {
        initialize_directory_simple(dir, "bigbed_file", "1.0");
        quick_text_write(bbpath, "foobar");
    }
    expect_validation_error(dir, "incorrect signature for a bigBed file");
}

TEST(BigBedFile, StrictError) {
    auto dir = define_test_path("bigbed_file");
    auto bbpath = (dir / "file.bb").string();

    {
        initialize_directory_simple(dir, "bigbed_file", "1.0");
        byteme::RawFileWriter handle(bbpath.c_str(), {});
        uint32_t val = 0xEBF28987;
        handle.write(reinterpret_cast<unsigned char*>(&val), sizeof(val));
    }

    takane::Options opts;
    opts.bigbed_file_strict_check = [](const std::filesystem::path&, const takane::ObjectMetadata&, const takane::Options&) { throw std::runtime_error("ARGH"); };
    expect_validation_error(dir, "ARGH", opts);
}
