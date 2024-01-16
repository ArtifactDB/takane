#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "takane/fasta_file.hpp"
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

    template<typename ... Args_>
    void expect_error(const std::string& msg, Args_&& ... args) {
        expect_validation_error(dir, msg, std::forward<Args_>(args)...);
    }
};

TEST_F(FastaFileTest, Basic) {
    initialize_directory_simple(dir, name, "2.0");
    expect_error("unsupported version");

    initialize_directory(dir);
    {
        std::ofstream ohandle(dir / "OBJECT");
        ohandle << "{ \"type\": \"" << name << "\", \"" << name << "\": { \"version\": \"1.0\", \"sequence_type\": \"DNA\" } }";
        byteme::GzipFileWriter handle(dir / "file.fasta.gz");
        handle.write("asdasd\nACGT\n");
    }
    expect_error("start with '>'");

    {
        byteme::GzipFileWriter handle(dir / "file.fasta.gz");
        handle.write(">asdasd\nACGT\n");
    }
    test_validate(dir);

    // Works with different sequence types.
    {
        std::ofstream ohandle(dir / "OBJECT");
        ohandle << "{ \"type\": \"" << name << "\", \"" << name << "\": { \"version\": \"1.0\", \"sequence_type\": \"RNA\" } }";
    }
    test_validate(dir);

    {
        std::ofstream ohandle(dir / "OBJECT");
        ohandle << "{ \"type\": \"" << name << "\", \"" << name << "\": { \"version\": \"1.0\", \"sequence_type\": \"AA\" } }";
    }
    test_validate(dir);

    {
        std::ofstream ohandle(dir / "OBJECT");
        ohandle << "{ \"type\": \"" << name << "\", \"" << name << "\": { \"version\": \"1.0\", \"sequence_type\": \"custom\" } }";
    }
    test_validate(dir);

    {
        std::ofstream ohandle(dir / "OBJECT");
        ohandle << "{ \"type\": \"" << name << "\", \"" << name << "\": { \"version\": \"1.0\", \"sequence_type\": \"foo\" } }";
    }
    expect_error("foo");
}

TEST_F(FastaFileTest, Indexed) {
    initialize_directory(dir);

    {
        std::ofstream ohandle(dir / "OBJECT");
        ohandle << "{ \"type\": \"" << name << "\", \"" << name << "\": { \"version\": \"1.0\", \"indexed\": true, \"sequence_type\": \"DNA\" } }";
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
        std::ofstream ihandle(dir / "file.fasta.fai");
        ihandle << "";
    }
    expect_error("missing BGZF index file");

    {
        std::ofstream ihandle(dir / "file.fasta.bgz.gzi");
        ihandle << "";
    }
    test_validate(dir);
}

TEST_F(FastaFileTest, Strict) {
    initialize_directory(dir);

    {
        std::ofstream ohandle(dir / "OBJECT");
        ohandle << "{ \"type\": \"" << name << "\", \"" << name << "\": { \"version\": \"1.0\", \"sequence_type\": \"DNA\" } }";
        byteme::GzipFileWriter fhandle(dir / "file.fasta.gz");
        fhandle.write(">asdasd\nACGT\n");
    }

    takane::Options opts;
    opts.fasta_file_strict_check = [](const std::filesystem::path&, const takane::ObjectMetadata&, const takane::Options&, bool) { throw std::runtime_error("ARGH"); };
    expect_error("ARGH", opts);
}
