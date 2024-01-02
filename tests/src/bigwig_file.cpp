#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "takane/bigwig_file.hpp"
#include "utils.h"

#include <string>
#include <vector>
#include <filesystem>
#include <stdexcept>

struct BigWigFileTest : public ::testing::Test {
    BigWigFileTest() {
        dir = "TEST_bigwig_file";
        name = "bigwig_file";
    }

    std::filesystem::path dir;
    std::string name;

    void expect_error(const std::string& msg) {
        EXPECT_ANY_THROW({
            try {
                test_validate(dir);
            } catch (std::exception& e) {
                EXPECT_THAT(e.what(), ::testing::HasSubstr(msg));
                throw;
            }
        });
    }
};

TEST_F(BigWigFileTest, Basic) {
    initialize_directory_simple(dir, name, "2.0");
    expect_error("unsupported version");

    initialize_directory_simple(dir, name, "1.0");
    {
        std::ofstream handle(dir / "file.bw");
        handle << "foobar";
    }
    expect_error("incorrect bigWig file signature");

    {
        byteme::RawFileWriter handle(dir / "file.bw");
        uint32_t val = 0x888FFC26;
        handle.write(reinterpret_cast<unsigned char*>(&val), sizeof(val));
    }
    test_validate(dir);

    {
        byteme::RawFileWriter handle(dir / "file.bw");
        uint32_t val = 0x26FC8F88;
        handle.write(reinterpret_cast<unsigned char*>(&val), sizeof(val));
    }
    test_validate(dir);
}

TEST_F(BigWigFileTest, Strict) {
    initialize_directory_simple(dir, name, "1.0");

    {
        byteme::RawFileWriter handle(dir / "file.bw");
        uint32_t val = 0x888FFC26;
        handle.write(reinterpret_cast<unsigned char*>(&val), sizeof(val));
    }

    takane::bigwig_file::strict_check = [](const std::filesystem::path&, const takane::ObjectMetadata&, const takane::Options&) { throw std::runtime_error("ARGH"); };
    expect_error("ARGH");
    takane::bigwig_file::strict_check = nullptr;
}
