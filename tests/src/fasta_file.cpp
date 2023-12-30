#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "takane/takane.hpp"
#include "utils.h"

#include <string>
#include <vector>
#include <filesystem>
#include <stdexcept>

struct FastaFileTest : public ::testing::Test {
    FastaFileTest() {
        dir = "TEST_fasta_file";
        name = "fasta_file";
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

TEST_F(FastaFileTest, Basic) {
    initialize_directory_simple(dir, name, "2.0");
    expect_error("unsupported version");

    initialize_directory_simple(dir, name, "1.0");
    {
        byteme::GzipFileWriter handle(dir / "file.fasta.gz");
        handle.write("asdasd\nACGT\n");
    }
    expect_error("start with '>'");

    {
        byteme::GzipFileWriter handle(dir / "file.fasta.gz");
        handle.write(">asdasd\nACGT\n");
    }
    takane::validate(dir);
}

TEST_F(FastaFileTest, Indexed) {
    initialize_directory(dir);

    {
        std::ofstream ohandle(dir / "OBJECT");
        ohandle << "{ \"type\": \"" << name << "\", \"" << name << "\": { \"version\": \"1.0\", \"indexed\": true } }";
        byteme::GzipFileWriter fhandle(dir / "file.fasta.bgz");
        fhandle.write("asdasd\nACGT\n");
    }
    expect_error("start with '>'");

    {
        byteme::GzipFileWriter fhandle(dir / "file.fasta.bgz");
        fhandle.write(">asdasd\nACGT\n");
    }
    expect_error("missing FASTA index file");

    {
        std::ofstream ihandle(dir / "file.fasta.bgz.fai");
        ihandle << "";
    }
    expect_error("missing BGZF index file");

    {
        std::ofstream ihandle(dir / "file.fasta.bgz.gzi");
        ihandle << "";
    }
    takane::validate(dir);
}

TEST_F(FastaFileTest, Strict) {
    initialize_directory_simple(dir, name, "1.0");

    {
        byteme::GzipFileWriter fhandle(dir / "file.fasta.gz");
        fhandle.write(">asdasd\nACGT\n");
    }

    takane::fasta_file::strict_check = [](const std::filesystem::path&, const takane::ObjectMetadata&, const takane::Options&, bool) { throw std::runtime_error("ARGH"); };
    expect_error("ARGH");
    takane::fasta_file::strict_check = nullptr;
}
