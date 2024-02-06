#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "takane/bed_file.hpp"
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

    template<typename ... Args_>
    void expect_error(const std::string& msg, Args_&& ... args) {
        expect_validation_error(dir, msg, std::forward<Args_>(args)...);
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
    test_validate(dir);
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
    test_validate(dir);
}

TEST_F(BedFileTest, Strict) {
    initialize_directory_simple(dir, name, "1.0");

    {
        byteme::GzipFileWriter fhandle(dir / "file.bed.gz");
        fhandle.write("chr1\t1\t2\n");
    }

    takane::Options opts;
    opts.bed_file_strict_check = [](const std::filesystem::path&, const takane::ObjectMetadata&, takane::Options&, bool) {};
    test_validate(dir);

    opts.bed_file_strict_check = [](const std::filesystem::path&, const takane::ObjectMetadata&, takane::Options&, bool) { throw std::runtime_error("ARGH"); };
    expect_error("ARGH", opts);
}
