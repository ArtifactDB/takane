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
    {
        std::ofstream ohandle(dir / "OBJECT");
        ohandle << "{ \"type\": \"" << name << "\", \"" << name << "\": { \"version\": \"1.0\", \"format\": true } }";
    }
    expect_error("JSON string");

    {
        std::ofstream ohandle(dir / "OBJECT");
        ohandle << "{ \"type\": \"" << name << "\", \"" << name << "\": { \"version\": \"1.0\", \"format\": \"FOO\" } }";
    }
    expect_error("unknown value");

    {
        std::ofstream ohandle(dir / "OBJECT");
        ohandle << "{ \"type\": \"" << name << "\", \"" << name << "\": { \"version\": \"1.0\", \"format\": \"GFF2\" } }";
        auto ggpath = (dir / "file.gff2.gz").string();
        byteme::GzipFileWriter fhandle(ggpath.c_str(), {});
        fhandle.write("chr1\t1\t2\n");
    }
    test_validate(dir);
}

TEST_F(GffFileTest, Basic3) {
    initialize_directory(dir);
    auto ggpath = (dir / "file.gff3.gz").string();
    {
        std::ofstream ohandle(dir / "OBJECT");
        ohandle << "{ \"type\": \"" << name << "\", \"" << name << "\": { \"version\": \"1.0\", \"format\": \"GFF3\" } }";
        byteme::GzipFileWriter fhandle(ggpath.c_str(), {});
        fhandle.write("chr1\t1\t2\n");
    }
    expect_error("GFF3 file signature");

    {
        byteme::GzipFileWriter handle(ggpath.c_str(), {});
        handle.write("##gff-version 3.1.26\nchr1\t1\t2\n");
    }
    test_validate(dir);
}

TEST_F(GffFileTest, Indexed) {
    initialize_directory(dir);

    {
        std::ofstream ohandle(dir / "OBJECT");
        ohandle << "{ \"type\": \"" << name << "\", \"" << name << "\": { \"version\": \"1.0\", \"format\": \"GFF2\", \"indexed\": true } }";
        auto ggpath = (dir / "file.gff2.bgz").string();
        byteme::GzipFileWriter fhandle(ggpath.c_str(), {});
        fhandle.write("chr1\t1\t2\n");
    }
    expect_error("failed to open");

    auto ggpath = (dir / "file.gff2.bgz.tbi").string();
    {
        byteme::GzipFileWriter ihandle(ggpath.c_str(), {});
        ihandle.write("foobar");
    }
    expect_error("tabix file signature");

    {
        byteme::GzipFileWriter ihandle(ggpath.c_str(), {});
        ihandle.write("TBI\1");
    }
    test_validate(dir);
}

TEST_F(GffFileTest, Strict) {
    initialize_directory(dir);
    {
        std::ofstream ohandle(dir / "OBJECT");
        ohandle << "{ \"type\": \"" << name << "\", \"" << name << "\": { \"version\": \"1.0\", \"format\": \"GFF2\" } }";
        auto ggpath = (dir / "file.gff2.gz").string();
        byteme::GzipFileWriter fhandle(ggpath.c_str(), {});
        fhandle.write("chr1\t1\t2\n");
    }

    takane::Options opts;
    opts.gff_file_strict_check = [](const std::filesystem::path&, const takane::ObjectMetadata&, takane::Options&, bool) {};
    test_validate(dir);

    opts.gff_file_strict_check = [](const std::filesystem::path&, const takane::ObjectMetadata&, takane::Options&, bool) { throw std::runtime_error("ARGH"); };
    expect_error("ARGH", opts);
}
