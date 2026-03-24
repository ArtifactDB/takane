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
    auto bdpath = (dir / "file.bed.gz").string();

    quick_text_write(bdpath, "WHEE");
    expect_error("GZIP file signature");

    quick_gzip_write(bdpath, "chr1\t1\t2\n");
    test_validate(dir);
}

TEST_F(BedFileTest, Indexed) {
    initialize_directory(dir);

    quick_text_write((dir / "OBJECT").string(), 
        "{ \"type\": \"" + name + "\", \"" + name + "\": { \"version\": \"1.0\", \"indexed\": true } }"
    );

    quick_gzip_write((dir / "file.bed.bgz").string(), "chr1\t1\t2\n");
    expect_error("failed to open");

    auto tbipath = (dir / "file.bed.bgz.tbi").string();
    quick_gzip_write(tbipath, "YAY");
    expect_error("tabix file signature");

    quick_gzip_write(tbipath, "TBI\1");
    test_validate(dir);
}

TEST_F(BedFileTest, Strict) {
    initialize_directory_simple(dir, name, "1.0");

    quick_gzip_write((dir / "file.bed.gz").string(), "chr1\t1\t2\n");

    takane::Options opts;
    opts.bed_file_strict_check = [](const std::filesystem::path&, const takane::ObjectMetadata&, takane::Options&, bool) {};
    test_validate(dir);

    opts.bed_file_strict_check = [](const std::filesystem::path&, const takane::ObjectMetadata&, takane::Options&, bool) { throw std::runtime_error("ARGH"); };
    expect_error("ARGH", opts);
}
