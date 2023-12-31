#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "takane/takane.hpp"
#include "utils.h"

#include <string>
#include <vector>
#include <filesystem>
#include <stdexcept>

struct GffFileTest : public ::testing::Test {
    GffFileTest() {
        dir = "TEST_gff_file";
        name = "gff_file";
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

TEST_F(GffFileTest, Basic2) {
    initialize_directory_simple(dir, name, "2.0");
    expect_error("unsupported version");

    initialize_directory(dir);
    {
        std::ofstream ohandle(dir / "OBJECT");
        ohandle << "{ \"type\": \"" << name << "\", \"" << name << "\": { \"version\": \"1.0\", \"format\": true } }";
    }
    expect_error("JSON string");

    {
        std::ofstream ohandle(dir / "OBJECT");
        ohandle << "{ \"type\": \"" << name << "\", \"" << name << "\": { \"version\": \"1.0\", \"format\": \"FOO\" } }";
    }
    expect_error("unknown value");

    {
        std::ofstream ohandle(dir / "OBJECT");
        ohandle << "{ \"type\": \"" << name << "\", \"" << name << "\": { \"version\": \"1.0\", \"format\": \"GFF2\" } }";
        byteme::GzipFileWriter fhandle(dir / "file.gff2.gz");
        fhandle.write("chr1\t1\t2\n");
    }
    takane::validate(dir);
}

TEST_F(GffFileTest, Basic3) {
    initialize_directory(dir);
    {
        std::ofstream ohandle(dir / "OBJECT");
        ohandle << "{ \"type\": \"" << name << "\", \"" << name << "\": { \"version\": \"1.0\", \"format\": \"GFF3\" } }";
        byteme::GzipFileWriter fhandle(dir / "file.gff3.gz");
        fhandle.write("chr1\t1\t2\n");
    }
    expect_error("GFF3 file signature");

    {
        byteme::GzipFileWriter handle(dir / "file.gff3.gz");
        handle.write("##gff-version 3.1.26\nchr1\t1\t2\n");
    }
    takane::validate(dir);
}

TEST_F(GffFileTest, Indexed) {
    initialize_directory(dir);

    {
        std::ofstream ohandle(dir / "OBJECT");
        ohandle << "{ \"type\": \"" << name << "\", \"" << name << "\": { \"version\": \"1.0\", \"format\": \"GFF2\", \"indexed\": true } }";
        byteme::GzipFileWriter fhandle(dir / "file.gff2.bgz");
        fhandle.write("chr1\t1\t2\n");
    }
    expect_error("failed to open");

    {
        byteme::GzipFileWriter ihandle(dir / "file.gff2.bgz.tbi");
        ihandle.write("foobar");
    }
    expect_error("tabix file signature");

    {
        byteme::GzipFileWriter ihandle(dir / "file.gff2.bgz.tbi");
        ihandle.write("TBI\1");
    }
    takane::validate(dir);
}

TEST_F(GffFileTest, Strict) {
    initialize_directory(dir);
    {
        std::ofstream ohandle(dir / "OBJECT");
        ohandle << "{ \"type\": \"" << name << "\", \"" << name << "\": { \"version\": \"1.0\", \"format\": \"GFF2\" } }";
        byteme::GzipFileWriter fhandle(dir / "file.gff2.gz");
        fhandle.write("chr1\t1\t2\n");
    }

    takane::gff_file::strict_check = [](const std::filesystem::path&, const takane::ObjectMetadata&, const takane::Options&, bool) { throw std::runtime_error("ARGH"); };
    expect_error("ARGH");
    takane::gff_file::strict_check = nullptr;
}
