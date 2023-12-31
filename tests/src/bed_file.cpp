#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "takane/takane.hpp"
#include "utils.h"

#include <string>
#include <vector>
#include <filesystem>
#include <stdexcept>

struct BedFileTest : public ::testing::Test {
    BedFileTest() {
        dir = "TEST_bed_file";
        name = "bed_file";
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

TEST_F(BedFileTest, Basic) {
    initialize_directory_simple(dir, name, "2.0");
    expect_error("unsupported version");

    initialize_directory_simple(dir, name, "1.0");
    {
        std::ofstream handle(dir / "file.bed.gz");
        handle << "WHEE";
    }
    expect_error("GZIP file signature");

    {
        byteme::GzipFileWriter handle(dir / "file.bed.gz");
        handle.write("chr1\t1\t2\n");
    }
    takane::validate(dir);
}

TEST_F(BedFileTest, Indexed) {
    initialize_directory(dir);

    {
        std::ofstream ohandle(dir / "OBJECT");
        ohandle << "{ \"type\": \"" << name << "\", \"" << name << "\": { \"version\": \"1.0\", \"indexed\": true } }";
        byteme::GzipFileWriter fhandle(dir / "file.bed.bgz");
        fhandle.write("chr1\t1\t2\n");
    }
    expect_error("failed to open");

    {
        byteme::GzipFileWriter ihandle(dir / "file.bed.bgz.tbi");
        ihandle.write("YAY");
    }
    expect_error("tabix file signature");

    {
        byteme::GzipFileWriter ihandle(dir / "file.bed.bgz.tbi");
        ihandle.write("TBI\1");
    }
    takane::validate(dir);
}

TEST_F(BedFileTest, Strict) {
    initialize_directory_simple(dir, name, "1.0");

    {
        byteme::GzipFileWriter fhandle(dir / "file.bed.gz");
        fhandle.write("chr1\t1\t2\n");
    }

    takane::bed_file::strict_check = [](const std::filesystem::path&, const takane::ObjectMetadata&, const takane::Options&, bool) { throw std::runtime_error("ARGH"); };
    expect_error("ARGH");
    takane::bed_file::strict_check = nullptr;
}
