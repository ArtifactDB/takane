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
    auto bampath = (dir / "file.bam").string();
    {
        std::ofstream handle(bampath.c_str(), {});
        handle << "FOO";
    }
    expect_error("incorrect GZIP file signature");

    {
        byteme::GzipFileWriter handle(bampath.c_str(), {});
        handle.write("foo\1");
    }
    expect_error("incorrect BAM file signature");

    {
        byteme::GzipFileWriter handle(bampath.c_str(), {});
        handle.write("BAM\1");
    }
    test_validate(dir);

    {
        std::ofstream handle(dir / "file.bam.bai");
        handle << "foobar\1";
    }
    expect_error("incorrect BAM index file signature");

    {
        std::ofstream handle(dir / "file.bam.bai");
        handle << "BAI\1";
    }
    test_validate(dir);

    auto csipath = (dir / "file.bam.csi").string();
    {
        byteme::GzipFileWriter handle(csipath.c_str(), {});
        handle.write("foobar\1");
    }
    expect_error("incorrect CSI index file signature");

    {
        byteme::GzipFileWriter handle(csipath.c_str(), {});
        handle.write("CSI\1");
    }
    test_validate(dir);
}

TEST_F(BamFileTest, Strict) {
    initialize_directory_simple(dir, name, "1.0");

    {
        auto bampath = (dir / "file.bam").string();
        byteme::GzipFileWriter handle(bampath.c_str(), {});
        handle.write("BAM\1");
    }

    takane::Options opts;
    opts.bam_file_strict_check = [](const std::filesystem::path&, const takane::ObjectMetadata&, takane::Options&) {};
    test_validate(dir);

    opts.bam_file_strict_check = [](const std::filesystem::path&, const takane::ObjectMetadata&, takane::Options&) { throw std::runtime_error("ARGH"); };
    expect_error("ARGH", opts);
}
