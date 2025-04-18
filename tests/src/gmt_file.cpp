#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "takane/gmt_file.hpp"
#include "utils.h"

#include <string>
#include <vector>
#include <filesystem>
#include <stdexcept>

struct GmtFileTest : public ::testing::Test {
    GmtFileTest() {
        dir = "TEST_gmt_file";
        name = "gmt_file";
    }

    std::filesystem::path dir;
    std::string name;

    template<typename ... Args_>
    void expect_error(const std::string& msg, Args_&& ... args) {
        expect_validation_error(dir, msg, std::forward<Args_>(args)...);
    }
};

TEST_F(GmtFileTest, Basic) {
    initialize_directory_simple(dir, name, "2.0");
    expect_error("unsupported version");

    initialize_directory_simple(dir, name, "1.0");
    {
        std::ofstream handle(dir / "file.gmt.gz");
        handle << "WHEE";
    }
    expect_error("GZIP file signature");

    {
        auto ggpath = (dir / "file.gmt.gz").string();
        byteme::GzipFileWriter handle(ggpath.c_str(), {});
        handle.write("set\tmy set\ta\tb\tc\n");
    }
    test_validate(dir);
}

TEST_F(GmtFileTest, Strict) {
    initialize_directory_simple(dir, name, "1.0");

    {
        auto ggpath = (dir / "file.gmt.gz").string();
        byteme::GzipFileWriter fhandle(ggpath.c_str(), {});
        fhandle.write("set\tmy set\ta\tb\tc\n");
    }

    takane::Options opts;
    opts.gmt_file_strict_check = [](const std::filesystem::path&, const takane::ObjectMetadata&, takane::Options&) {};
    test_validate(dir);

    opts.gmt_file_strict_check = [](const std::filesystem::path&, const takane::ObjectMetadata&, takane::Options&) { throw std::runtime_error("ARGH"); };
    expect_error("ARGH", opts);
}
