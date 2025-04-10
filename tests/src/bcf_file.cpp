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
    {
        std::ofstream handle(bcfpath.c_str(), {});
        handle << "foo\1";
    }
    expect_error("incorrect GZIP file signature");

    {
        byteme::GzipFileWriter handle(bcfpath.c_str(), {});
        handle.write("foobar\2\1");
    }
    expect_error("incorrect BCF file signature");

    {
        byteme::GzipFileWriter handle(bcfpath.c_str(), {});
        handle.write("BCF\2\1");
    }
    test_validate(dir);

    auto tbipath = (dir / "file.bcf.tbi").string();
    {
        byteme::GzipFileWriter handle(tbipath.c_str(), {});
        handle.write("foobar\1");
    }
    expect_error("incorrect tabix file signature");

    {
        byteme::GzipFileWriter handle(tbipath.c_str(), {});
        handle.write("TBI\1");
    }
    test_validate(dir);

    auto csipath = (dir / "file.bcf.csi").string();
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

TEST_F(BcfFileTest, Strict) {
    initialize_directory_simple(dir, name, "1.0");
    auto bcfpath = (dir / "file.bcf").string();
    {
        byteme::GzipFileWriter handle(bcfpath.c_str(), {});
        handle.write("BCF\2\1");
    }

    takane::Options opts;
    opts.bcf_file_strict_check = [](const std::filesystem::path&, const takane::ObjectMetadata&, takane::Options&) {};
    test_validate(dir);

    opts.bcf_file_strict_check = [](const std::filesystem::path&, const takane::ObjectMetadata&, takane::Options&) { throw std::runtime_error("ARGH"); };
    expect_error("ARGH", opts);
}
