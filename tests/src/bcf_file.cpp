#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "takane/bcf_file.hpp"
#include "utils.h"

#include <string>
#include <vector>
#include <filesystem>
#include <stdexcept>

struct BcfFileTest : public ::testing::Test {
    BcfFileTest() {
        dir = "TEST_bcf_file";
        name = "bcf_file";
    }

    std::filesystem::path dir;
    std::string name;

    template<typename ... Args_>
    void expect_error(const std::string& msg, Args_&& ... args) {
        expect_validation_error(dir, msg, std::forward<Args_>(args)...);
    }
};

TEST_F(BcfFileTest, Basic) {
    initialize_directory_simple(dir, name, "2.0");
    expect_error("unsupported version");

    initialize_directory_simple(dir, name, "1.0");
    auto bcfpath = (dir / "file.bcf").string();

    quick_text_write(bcfpath, "foo\1");
    expect_error("incorrect GZIP file signature");

    quick_gzip_write(bcfpath, "foobar\2\1");
    expect_error("incorrect BCF file signature");

    quick_gzip_write(bcfpath, "BCF\2\1");
    test_validate(dir);

    auto tbipath = (dir / "file.bcf.tbi").string();
    quick_gzip_write(tbipath, "foobar\1");
    expect_error("incorrect tabix file signature");

    quick_gzip_write(tbipath, "TBI\1");
    test_validate(dir);

    auto csipath = (dir / "file.bcf.csi").string();
    quick_gzip_write(csipath, "foobar\1");
    expect_error("incorrect CSI index file signature");

    quick_gzip_write(csipath, "CSI\1");
    test_validate(dir);
}

TEST_F(BcfFileTest, Strict) {
    initialize_directory_simple(dir, name, "1.0");

    auto bcfpath = (dir / "file.bcf").string();
    quick_gzip_write(bcfpath, "BCF\2\1");

    takane::Options opts;
    opts.bcf_file_strict_check = [](const std::filesystem::path&, const takane::ObjectMetadata&, takane::Options&) {};
    test_validate(dir);

    opts.bcf_file_strict_check = [](const std::filesystem::path&, const takane::ObjectMetadata&, takane::Options&) { throw std::runtime_error("ARGH"); };
    expect_error("ARGH", opts);
}
