#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "takane/rds_file.hpp"
#include "utils.h"

#include <string>
#include <vector>
#include <filesystem>
#include <stdexcept>

struct RdsFileTest : public ::testing::Test {
    RdsFileTest() {
        dir = "TEST_rds_file";
        name = "rds_file";
    }

    std::filesystem::path dir;
    std::string name;

    template<typename ... Args_>
    void expect_error(const std::string& msg, Args_&& ... args) {
        expect_validation_error(dir, msg, std::forward<Args_>(args)...);
    }
};

TEST_F(RdsFileTest, Basic) {
    initialize_directory_simple(dir, name, "2.0");
    expect_error("unsupported version");

    initialize_directory_simple(dir, name, "1.0");
    {
        byteme::GzipFileWriter fhandle(dir / "file.rds");
        fhandle.write("X");
    }
    expect_error("incomplete");

    {
        byteme::GzipFileWriter fhandle(dir / "file.rds");
        fhandle.write("B\n");
    }
    expect_error("incorrect");

    {
        byteme::GzipFileWriter fhandle(dir / "file.rds");
        fhandle.write("X\n");
    }
    test_validate(dir);
}

TEST_F(RdsFileTest, Strict) {
    initialize_directory_simple(dir, name, "1.0");
    {
        byteme::GzipFileWriter fhandle(dir / "file.rds");
        fhandle.write("X\n");
    }

    takane::Options opts;
    opts.rds_file_strict_check = [](const std::filesystem::path&, const takane::ObjectMetadata&, const takane::Options&) {};
    test_validate(dir);

    opts.rds_file_strict_check = [](const std::filesystem::path&, const takane::ObjectMetadata&, const takane::Options&) { throw std::runtime_error("ARGH"); };
    expect_error("ARGH", opts);
}
