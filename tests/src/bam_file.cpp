#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "takane/takane.hpp"
#include "utils.h"

#include <string>
#include <vector>
#include <filesystem>
#include <stdexcept>

struct BamFileTest : public ::testing::Test {
    BamFileTest() {
        dir = "TEST_bam_file";
        name = "bam_file";
    }

    std::filesystem::path dir;
    std::string name;

    void expect_error(const std::string& msg) {
        EXPECT_ANY_THROW({
            try {
                takane::validate(dir);
            } catch (std::exception& e) {
                EXPECT_THAT(e.what(), ::testing::HasSubstr(msg));
                throw;
            }
        });
    }
};

TEST_F(BamFileTest, Basic) {
    initialize_directory_simple(dir, name, "2.0");
    expect_error("unsupported version");

    initialize_directory_simple(dir, name, "1.0");
    {
        std::ofstream handle(dir / "file.bam");
        handle << "foo\1";
    }
    expect_error("incorrect BAM file signature");

    {
        std::ofstream handle(dir / "file.bam");
        handle << "BAM\1";
    }
    takane::validate(dir);

    {
        std::ofstream handle(dir / "file.bam.bai");
        handle << "foobar\1";
    }
    expect_error("incorrect BAM index file signature");

    {
        std::ofstream handle(dir / "file.bam.bai");
        handle << "BAI\1";
    }
    takane::validate(dir);

    {
        std::ofstream handle(dir / "file.bam.csi");
        handle << "foobar\1";
    }
    expect_error("incorrect CSI index file signature");

    {
        std::ofstream handle(dir / "file.bam.csi");
        handle << "CSI\1";
    }
    takane::validate(dir);
}

TEST_F(BamFileTest, Strict) {
    initialize_directory_simple(dir, name, "1.0");

    {
        std::ofstream handle(dir / "file.bam");
        handle << "BAM\1";
    }

    takane::bam_file::strict_check = [](const std::filesystem::path&, const takane::ObjectMetadata&, const takane::Options&) { throw std::runtime_error("ARGH"); };
    expect_error("ARGH");
    takane::bam_file::strict_check = nullptr;
}