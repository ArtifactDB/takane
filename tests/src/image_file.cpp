#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "takane/image_file.hpp"
#include "utils.h"

#include <string>
#include <vector>
#include <filesystem>
#include <stdexcept>

struct ImageFileTest : public ::testing::Test {
    ImageFileTest() {
        dir = "TEST_image_file";
        name = "image_file";
    }

    std::filesystem::path dir;
    std::string name;

    template<typename ... Args_>
    void expect_error(const std::string& msg, Args_&& ... args) {
        expect_validation_error(dir, msg, std::forward<Args_>(args)...);
    }

    void dump_object_file(const std::string& format) {
        auto obody = "{ \"type\": \"image_file\", \"image_file\": { \"version\": \"1.0\", \"format\": \"" + format + "\" } }";
        auto opath = dir / "OBJECT"; 
        byteme::RawFileWriter writer(opath.c_str(), {});
        writer.write(reinterpret_cast<const unsigned char*>(obody.c_str()), obody.size());
    }
};

TEST_F(ImageFileTest, Basic) {
    initialize_directory_simple(dir, name, "2.0");
    expect_error("unsupported version");

    initialize_directory(dir);
    dump_object_file("FOO");
    expect_error("unsupported format");
}

TEST_F(ImageFileTest, Png) {
    initialize_directory(dir);
    dump_object_file("PNG");

    auto ipath = dir / "file.png";
    {
        std::ofstream ohandle(ipath);
    }
    expect_error("incomplete PNG file signature");

    {
        std::ofstream ohandle(ipath);
        ohandle << "chino-chan";
    }
    expect_error("incorrect PNG file signature");

    {
        std::ofstream ohandle(ipath);
        constexpr std::array<unsigned char, 8> stuff { 137, 80, 78, 71, 13, 10, 26, 10 };
        ohandle.write(reinterpret_cast<const char*>(stuff.data()), stuff.size());
    }
    test_validate(dir);

    // Adding a strict check.
    takane::Options opt;
    opt.image_file_strict_check = [&](const std::filesystem::path&, const takane::ObjectMetadata&, takane::Options&) -> void {
        throw std::runtime_error("FOOBAR");
    };
    expect_validation_error(dir, "FOOBAR", opt);
}

TEST_F(ImageFileTest, Tiff) {
    initialize_directory(dir);
    dump_object_file("TIFF");

    auto ipath = dir / "file.tif";
    {
        std::ofstream ohandle(ipath);
    }
    expect_error("too small");

    {
        std::ofstream ohandle(ipath);
        ohandle << "chino-chan";
    }
    expect_error("incorrect TIFF file signature");

    {
        std::ofstream ohandle(ipath);
        constexpr std::array<unsigned char, 4> stuff{ 0x49, 0x49, 0x2A, 0x00 };
        ohandle.write(reinterpret_cast<const char*>(stuff.data()), stuff.size());
    }
    test_validate(dir);

    {
        std::ofstream ohandle(ipath);
        constexpr std::array<unsigned char, 4> stuff{ 0x4D, 0x4D, 0x00, 0x2A };
        ohandle.write(reinterpret_cast<const char*>(stuff.data()), stuff.size());
    }
    test_validate(dir);
}

TEST_F(ImageFileTest, Jpeg) {
    initialize_directory(dir);
    dump_object_file("JPEG");

    auto ipath = dir / "file.jpg";
    {
        std::ofstream ohandle(ipath);
    }
    expect_error("incomplete JPEG file signature");

    {
        std::ofstream ohandle(ipath);
        ohandle << "chino-chan";
    }
    expect_error("incorrect JPEG file signature");

    {
        std::ofstream ohandle(ipath);
        constexpr std::array<unsigned char, 4> stuff { 0xff, 0xd8, 0xff, 0xe1 };
        ohandle.write(reinterpret_cast<const char*>(stuff.data()), stuff.size());
    }
    test_validate(dir);
}

TEST_F(ImageFileTest, Gif) {
    initialize_directory(dir);
    dump_object_file("GIF");

    auto ipath = dir / "file.gif";
    {
        std::ofstream ohandle(ipath);
    }
    expect_error("incomplete GIF file signature");

    {
        std::ofstream ohandle(ipath);
        ohandle << "chino-chan";
    }
    expect_error("incorrect GIF file signature");

    {
        std::ofstream ohandle(ipath);
        constexpr std::array<unsigned char, 4> stuff { 0x47, 0x49, 0x46, 0x38 };
        ohandle.write(reinterpret_cast<const char*>(stuff.data()), stuff.size());
    }
    test_validate(dir);
}

TEST_F(ImageFileTest, Webp) {
    initialize_directory(dir);
    dump_object_file("WEBP");

    auto ipath = dir / "file.webp";
    {
        std::ofstream ohandle(ipath);
    }
    expect_error("too small");

    {
        std::ofstream ohandle(ipath);
        ohandle << "kirima-syaro";
    }
    expect_error("incorrect WEBP file signature");

    {
        std::ofstream ohandle(ipath);
        constexpr std::array<unsigned char, 12> stuff { 0x52, 0x49, 0x46, 0x46, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 };
        ohandle.write(reinterpret_cast<const char*>(stuff.data()), stuff.size());
    }
    expect_error("incorrect WEBP file signature");

    {
        std::ofstream ohandle(ipath);
        constexpr std::array<unsigned char, 12> stuff { 0x52, 0x49, 0x46, 0x46, 0x0, 0x0, 0x0, 0x0, 0x57, 0x45, 0x42, 0x50 };
        ohandle.write(reinterpret_cast<const char*>(stuff.data()), stuff.size());
    }
    test_validate(dir);
}
