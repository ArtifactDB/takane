#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "takane/takane.hpp"
#include "utils.h"

#include <string>
#include <vector>
#include <filesystem>
#include <stdexcept>

struct FastqFileTest : public ::testing::Test {
    FastqFileTest() {
        dir = "TEST_fastq_file";
        name = "fastq_file";
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

TEST_F(FastqFileTest, Basic) {
    initialize_directory_simple(dir, name, "2.0");
    expect_error("unsupported version");

    initialize_directory_simple(dir, name, "1.0");
    {
        byteme::GzipFileWriter handle(dir / "file.fastq.gz");
        handle.write("asdasd\nACGT\n+\n!!!!\n");
    }
    expect_error("start with '@'");

    {
        byteme::GzipFileWriter handle(dir / "file.fastq.gz");
        handle.write("@asdasd\nACGT\n+\n!!!!\n");
    }
    takane::validate(dir);

    // Checking the quality offset.
    {
        std::ofstream ohandle(dir / "OBJECT");
        ohandle << "{ \"type\": \"" << name << "\", \"" << name << "\": { \"version\": \"1.0\", \"quality_offset\": false } }";
    }
    expect_error("JSON number");

    {
        std::ofstream ohandle(dir / "OBJECT");
        ohandle << "{ \"type\": \"" << name << "\", \"" << name << "\": { \"version\": \"1.0\", \"quality_offset\": 20 } }";
    }
    expect_error("33 or 64");

    {
        std::ofstream ohandle(dir / "OBJECT");
        ohandle << "{ \"type\": \"" << name << "\", \"" << name << "\": { \"version\": \"1.0\", \"quality_offset\": 64 } }";
    }
    takane::validate(dir);
}

TEST_F(FastqFileTest, Indexed) {
    initialize_directory(dir);

    {
        std::ofstream ohandle(dir / "OBJECT");
        ohandle << "{ \"type\": \"" << name << "\", \"" << name << "\": { \"version\": \"1.0\", \"indexed\": true } }";
        byteme::GzipFileWriter fhandle(dir / "file.fastq.bgz");
        fhandle.write("asdasd\nACGT\n+\n!!!!\n");
    }
    expect_error("start with '@'");

    {
        byteme::GzipFileWriter fhandle(dir / "file.fastq.bgz");
        fhandle.write("@asdasd\nACGT\n+\n!!!!\n");
    }
    expect_error("missing FASTQ index file");

    {
        std::ofstream ihandle(dir / "file.fastq.bgz.fai");
        ihandle << "";
    }
    expect_error("missing BGZF index file");

    {
        std::ofstream ihandle(dir / "file.fastq.bgz.gzi");
        ihandle << "";
    }
    takane::validate(dir);
}

TEST_F(FastqFileTest, Strict) {
    initialize_directory_simple(dir, name, "1.0");

    {
        byteme::GzipFileWriter fhandle(dir / "file.fastq.gz");
        fhandle.write("@asdasd\nACGT\n+\n!!!!\n");
    }

    takane::fastq_file::strict_check = [](const std::filesystem::path&, const takane::ObjectMetadata&, const takane::Options&, bool) { throw std::runtime_error("ARGH"); };
    expect_error("ARGH");
    takane::fastq_file::strict_check = nullptr;
}
