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

TEST_F(BcfFileTest, Basic) {
    initialize_directory_simple(dir, name, "2.0");
    expect_error("unsupported version");

    initialize_directory_simple(dir, name, "1.0");
    {
        std::ofstream handle(dir / "file.bcf");
        handle << "foo\1";
    }
    expect_error("incorrect GZIP file signature");

    {
        byteme::GzipFileWriter handle(dir / "file.bcf");
        handle.write("foobar\2\1");
    }
    expect_error("incorrect BCF file signature");

    {
        byteme::GzipFileWriter handle(dir / "file.bcf");
        handle.write("BCF\2\1");
    }
    test_validate(dir);

    {
        byteme::GzipFileWriter handle(dir / "file.bcf.tbi");
        handle.write("foobar\1");
    }
    expect_error("incorrect tabix file signature");

    {
        byteme::GzipFileWriter handle(dir / "file.bcf.tbi");
        handle.write("TBI\1");
    }
    test_validate(dir);

    {
        byteme::GzipFileWriter handle(dir / "file.bcf.csi");
        handle.write("foobar\1");
    }
    expect_error("incorrect CSI index file signature");

    {
        byteme::GzipFileWriter handle(dir / "file.bcf.csi");
        handle.write("CSI\1");
    }
    test_validate(dir);
}

TEST_F(BcfFileTest, Strict) {
    initialize_directory_simple(dir, name, "1.0");

    {
        byteme::GzipFileWriter handle(dir / "file.bcf");
        handle.write("BCF\2\1");
    }

    takane::bcf_file::strict_check = [](const std::filesystem::path&, const takane::ObjectMetadata&, const takane::Options&) { throw std::runtime_error("ARGH"); };
    expect_error("ARGH");
    takane::bcf_file::strict_check = nullptr;
}
