#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "takane/fastq_file.hpp"
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

    template<typename ... Args_>
    void expect_error(const std::string& msg, Args_&& ... args) {
        expect_validation_error(dir, msg, std::forward<Args_>(args)...);
    }
};

TEST_F(FastqFileTest, Basic) {
    initialize_directory_simple(dir, name, "2.0");
    expect_error("unsupported version");

    initialize_directory(dir);
    auto fqpath = (dir / "file.fastq.gz").string();
    {
        std::ofstream ohandle(dir / "OBJECT");
        ohandle << "{ \"type\": \"" << name << "\", \"" << name << "\": { \"version\": \"1.0\", \"sequence_type\": \"DNA\", \"quality_type\": \"phred\", \"quality_offset\": 33 } }";
        byteme::GzipFileWriter handle(fqpath.c_str(), {});
        handle.write("asdasd\nACGT\n+\n!!!!\n");
    }
    expect_error("start with '@'");

    {
        byteme::GzipFileWriter handle(fqpath.c_str(), {});
        handle.write("@asdasd\nACGT\n+\n!!!!\n");
    }
    test_validate(dir);

    // Checking the metadata categories.
    {
        std::ofstream ohandle(dir / "OBJECT");
        ohandle << "{ \"type\": \"" << name << "\", \"" << name << "\": { \"version\": \"1.0\" } }";
    }
    expect_error("sequence_type");

    {
        std::ofstream ohandle(dir / "OBJECT");
        ohandle << "{ \"type\": \"" << name << "\", \"" << name << "\": { \"version\": \"1.0\", \"sequence_type\": \"DNA\" } }";
    }
    expect_error("quality_type");

    {
        std::ofstream ohandle(dir / "OBJECT");
        ohandle << "{ \"type\": \"" << name << "\", \"" << name << "\": { \"version\": \"1.0\", \"sequence_type\": \"DNA\", \"quality_type\": true } }";
    }
    expect_error("JSON string");

    {
        std::ofstream ohandle(dir / "OBJECT");
        ohandle << "{ \"type\": \"" << name << "\", \"" << name << "\": { \"version\": \"1.0\", \"sequence_type\": \"DNA\", \"quality_type\": \"foo\" } }";
    }
    expect_error("unknown value 'foo'");

    {
        std::ofstream ohandle(dir / "OBJECT");
        ohandle << "{ \"type\": \"" << name << "\", \"" << name << "\": { \"version\": \"1.0\", \"sequence_type\": \"DNA\", \"quality_type\": \"solexa\" } }";
    }
    test_validate(dir);

    // Checking the quality offset.
    {
        std::ofstream ohandle(dir / "OBJECT");
        ohandle << "{ \"type\": \"" << name << "\", \"" << name << "\": { \"version\": \"1.0\", \"sequence_type\": \"DNA\", \"quality_type\": \"phred\" } }";
    }
    expect_error("quality_offset");

    {
        std::ofstream ohandle(dir / "OBJECT");
        ohandle << "{ \"type\": \"" << name << "\", \"" << name << "\": { \"version\": \"1.0\", \"sequence_type\": \"DNA\", \"quality_type\": \"phred\", \"quality_offset\": true } }";
    }
    expect_error("JSON number");

    {
        std::ofstream ohandle(dir / "OBJECT");
        ohandle << "{ \"type\": \"" << name << "\", \"" << name << "\": { \"version\": \"1.0\", \"sequence_type\": \"DNA\", \"quality_type\": \"phred\", \"quality_offset\": 20 } }";
    }
    expect_error("33 or 64");

    {
        std::ofstream ohandle(dir / "OBJECT");
        ohandle << "{ \"type\": \"" << name << "\", \"" << name << "\": { \"version\": \"1.0\", \"sequence_type\": \"DNA\", \"quality_type\": \"phred\", \"quality_offset\": 64 } }";
    }
    test_validate(dir);
}

TEST_F(FastqFileTest, Indexed) {
    initialize_directory(dir);
    auto fqpath = (dir / "file.fastq.bgz").string();
    {
        std::ofstream ohandle(dir / "OBJECT");
        ohandle << "{ \"type\": \"" << name << "\", \"" << name << "\": { \"version\": \"1.0\", \"indexed\": true, \"sequence_type\": \"DNA\", \"quality_type\": \"solexa\" } }";
        byteme::GzipFileWriter fhandle(fqpath.c_str(), {});
        fhandle.write("asdasd\nACGT\n+\n!!!!\n");
    }
    expect_error("start with '@'");

    {
        byteme::GzipFileWriter fhandle(fqpath.c_str(), {});
        fhandle.write("@asdasd\nACGT\n+\n!!!!\n");
    }
    expect_error("missing FASTQ index file");

    {
        std::ofstream ihandle(dir / "file.fastq.fai");
        ihandle << "";
    }
    expect_error("missing BGZF index file");

    {
        std::ofstream ihandle(dir / "file.fastq.bgz.gzi");
        ihandle << "";
    }
    test_validate(dir);
}

TEST_F(FastqFileTest, Strict) {
    initialize_directory(dir);
    auto fqpath = (dir / "file.fastq.gz").string();
    {
        std::ofstream ohandle(dir / "OBJECT");
        ohandle << "{ \"type\": \"" << name << "\", \"" << name << "\": { \"version\": \"1.0\", \"sequence_type\": \"DNA\", \"quality_type\": \"phred\", \"quality_offset\": 64 } }";
        byteme::GzipFileWriter fhandle(fqpath.c_str(), {});
        fhandle.write("@asdasd\nACGT\n+\n!!!!\n");
    }

    takane::Options opts;
    opts.fastq_file_strict_check = [](const std::filesystem::path&, const takane::ObjectMetadata&, takane::Options&, bool) {};
    test_validate(dir);

    opts.fastq_file_strict_check = [](const std::filesystem::path&, const takane::ObjectMetadata&, takane::Options&, bool) { throw std::runtime_error("ARGH"); };
    expect_error("ARGH", opts);
}
