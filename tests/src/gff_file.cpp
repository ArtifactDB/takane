#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "takane/gff_file.hpp"
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

    template<typename ... Args_>
    void expect_error(const std::string& msg, Args_&& ... args) {
        expect_validation_error(dir, msg, std::forward<Args_>(args)...);
    }
};

TEST_F(GffFileTest, Basic2) {
    initialize_directory_simple(dir, name, "2.0");
    expect_error("unsupported version");

    initialize_directory(dir);
    auto objpath = (dir / "OBJECT").string();

    quick_text_write(objpath, 
        "{ \"type\": \"" + name + "\", \"" + name + "\": { \"version\": \"1.0\", \"format\": true } }"
    );
    expect_error("JSON string");

    quick_text_write(objpath, 
        "{ \"type\": \"" + name + "\", \"" + name + "\": { \"version\": \"1.0\", \"format\": \"FOO\" } }"
    );
    expect_error("unknown value");

    quick_text_write(objpath, 
        "{ \"type\": \"" + name + "\", \"" + name + "\": { \"version\": \"1.0\", \"format\": \"GFF2\" } }"
    );
    quick_gzip_write((dir / "file.gff2.gz").string(), "chr1\t1\t2\n");
    test_validate(dir);
}

TEST_F(GffFileTest, Basic3) {
    initialize_directory(dir);
    auto ggpath = (dir / "file.gff3.gz").string();

    quick_text_write((dir / "OBJECT").string(),
        "{ \"type\": \"" + name + "\", \"" + name + "\": { \"version\": \"1.0\", \"format\": \"GFF3\" } }"
    );
    quick_gzip_write(ggpath, "chr1\t1\t2\n");
    expect_error("GFF3 file signature");

    quick_gzip_write(ggpath, "##gff-version 3.1.26\nchr1\t1\t2\n");
    test_validate(dir);
}

TEST_F(GffFileTest, Indexed) {
    initialize_directory(dir);

    quick_text_write((dir / "OBJECT").string(),
        "{ \"type\": \"" + name + "\", \"" + name + "\": { \"version\": \"1.0\", \"format\": \"GFF2\", \"indexed\": true } }"
    );
    quick_gzip_write((dir / "file.gff2.bgz").string(), "chr1\t1\t2\n");
    expect_error("failed to open");

    auto ipath = (dir / "file.gff2.bgz.tbi").string();
    quick_gzip_write(ipath, "foobar");
    expect_error("tabix file signature");

    quick_gzip_write(ipath, "TBI\1");
    test_validate(dir);
}

TEST_F(GffFileTest, Strict) {
    initialize_directory(dir);

    quick_text_write((dir / "OBJECT").string(),
        "{ \"type\": \"" + name + "\", \"" + name + "\": { \"version\": \"1.0\", \"format\": \"GFF2\" } }"
    );
    quick_gzip_write((dir / "file.gff2.gz").string(), "chr1\t1\t2\n");

    takane::Options opts;
    opts.gff_file_strict_check = [](const std::filesystem::path&, const takane::ObjectMetadata&, takane::Options&, bool) {};
    test_validate(dir);

    opts.gff_file_strict_check = [](const std::filesystem::path&, const takane::ObjectMetadata&, takane::Options&, bool) { throw std::runtime_error("ARGH"); };
    expect_error("ARGH", opts);
}
