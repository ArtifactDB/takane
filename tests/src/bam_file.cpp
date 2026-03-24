#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "takane/bam_file.hpp"
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

    template<typename ... Args_>
    void expect_error(const std::string& msg, Args_&& ... args) {
        expect_validation_error(dir, msg, std::forward<Args_>(args)...);
    }
};

TEST_F(BamFileTest, Basic) {
    initialize_directory_simple(dir, name, "2.0");
    expect_error("unsupported version");

    initialize_directory_simple(dir, name, "1.0");

    {
        auto bampath = (dir / "file.bam").string();
        quick_text_write(bampath, "FOO");
        expect_error("incorrect GZIP file signature");

        quick_gzip_write(bampath, "foo\1");
        expect_error("incorrect BAM file signature");

        quick_gzip_write(bampath, "BAM\1");
        test_validate(dir);
    }

    {
        auto baipath = (dir / "file.bam.bai").string();
        quick_text_write(baipath, "foobar\1");
        expect_error("incorrect BAM index file signature");

        quick_text_write(baipath, "BAI\1");
        test_validate(dir);
    }

    {
        auto csipath = (dir / "file.bam.csi").string();
        quick_text_write(csipath, "FOO");
        expect_error("incorrect GZIP file signature");

        quick_gzip_write(csipath, "foobar\1");
        expect_error("incorrect CSI index file signature");

        quick_gzip_write(csipath, "CSI\1");
        test_validate(dir);
    }
}

TEST_F(BamFileTest, Strict) {
    initialize_directory_simple(dir, name, "1.0");

    auto bampath = (dir / "file.bam").string();
    quick_gzip_write(bampath, "BAM\1");

    takane::Options opts;
    opts.bam_file_strict_check = [](const std::filesystem::path&, const takane::ObjectMetadata&, takane::Options&) {};
    test_validate(dir);

    opts.bam_file_strict_check = [](const std::filesystem::path&, const takane::ObjectMetadata&, takane::Options&) { throw std::runtime_error("ARGH"); };
    expect_error("ARGH", opts);
}
