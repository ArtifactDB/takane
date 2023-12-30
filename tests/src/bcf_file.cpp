#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "takane/takane.hpp"
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

TEST_F(BcfFileTest, Basic) {
    initialize_directory_simple(dir, name, "2.0");
    expect_error("unsupported version");

    initialize_directory_simple(dir, name, "1.0");
    {
        std::ofstream handle(dir / "file.bcf");
        handle << "foo\1";
    }
    expect_error("incorrect BCF file signature");

    {
        std::ofstream handle(dir / "file.bcf");
        handle << "BCF\2\1";
    }
    takane::validate(dir);

    {
        std::ofstream handle(dir / "file.bcf.tbi");
        handle << "foobar\1";
    }
    expect_error("incorrect TBI index file signature");

    {
        std::ofstream handle(dir / "file.bcf.tbi");
        handle << "TBI\1";
    }
    takane::validate(dir);

    {
        std::ofstream handle(dir / "file.bcf.csi");
        handle << "foobar\1";
    }
    expect_error("incorrect CSI index file signature");

    {
        std::ofstream handle(dir / "file.bcf.csi");
        handle << "CSI\1";
    }
    takane::validate(dir);
}

TEST_F(BcfFileTest, Strict) {
    initialize_directory_simple(dir, name, "1.0");

    {
        std::ofstream handle(dir / "file.bcf");
        handle << "BCF\2\1";
    }

    takane::bcf_file::strict_check = [](const std::filesystem::path&, const takane::ObjectMetadata&, const takane::Options&) { throw std::runtime_error("ARGH"); };
    expect_error("ARGH");
    takane::bcf_file::strict_check = nullptr;
}
