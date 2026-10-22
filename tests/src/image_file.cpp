#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "takane/image_file.hpp"
#include "utils.h"

#include <string>
#include <vector>
#include <filesystem>
#include <stdexcept>

TEST(ImageFile, PngOkay) {
    auto dir = define_test_path("image_file");

    {
        initialize_directory(dir);
        quick_text_write(
            dir / "OBJECT",
            "{ \"type\": \"image_file\", \"image_file\": { \"version\": \"1.0\", \"format\": \"PNG\" } }"
        );
        std::ofstream ohandle(dir / "file.png");
        constexpr std::array<unsigned char, 8> stuff { 137, 80, 78, 71, 13, 10, 26, 10 };
        ohandle.write(reinterpret_cast<const char*>(stuff.data()), stuff.size());
    }
    test_validate(dir);
}

TEST(ImageFile, PngError) {
    auto dir = define_test_path("image_file");

    {
        initialize_directory(dir);
        quick_text_write(
            dir / "OBJECT",
            "{ \"type\": \"image_file\", \"image_file\": { \"version\": \"1.0\", \"format\": \"PNG\" } }"
        );
        quick_text_write(dir / "file.png", "chino-chan");
    }
    expect_validation_error(dir, "PNG file");
}

/***********************************************/

TEST(ImageFile, TiffOkay) {
    auto dir = define_test_path("image_file");

    {
        initialize_directory(dir);
        quick_text_write(
            dir / "OBJECT",
            "{ \"type\": \"image_file\", \"image_file\": { \"version\": \"1.0\", \"format\": \"TIFF\" } }"
        );
        std::ofstream ohandle(dir / "file.tif");
        constexpr std::array<unsigned char, 4> stuff{ 0x49, 0x49, 0x2A, 0x00 };
        ohandle.write(reinterpret_cast<const char*>(stuff.data()), stuff.size());
    }
    test_validate(dir);

    {
        initialize_directory(dir);
        quick_text_write(
            dir / "OBJECT",
            "{ \"type\": \"image_file\", \"image_file\": { \"version\": \"1.0\", \"format\": \"TIFF\" } }"
        );
        std::ofstream ohandle(dir / "file.tif");
        constexpr std::array<unsigned char, 4> stuff{ 0x4D, 0x4D, 0x00, 0x2A };
        ohandle.write(reinterpret_cast<const char*>(stuff.data()), stuff.size());
    }
    test_validate(dir);
}

TEST(ImageFile, TiffError) {
    auto dir = define_test_path("image_file");

    {
        initialize_directory(dir);
        quick_text_write(
            dir / "OBJECT",
            "{ \"type\": \"image_file\", \"image_file\": { \"version\": \"1.0\", \"format\": \"TIFF\" } }"
        );
        quick_text_write(dir / "file.tif", "chino-chan");
    }
    expect_validation_error(dir, "TIFF file");
}

/***********************************************/

TEST(ImageFile, JpegOkay) {
    auto dir = define_test_path("image_file");

    {
        initialize_directory(dir);
        quick_text_write(
            dir / "OBJECT",
            "{ \"type\": \"image_file\", \"image_file\": { \"version\": \"1.0\", \"format\": \"JPEG\" } }"
        );
        std::ofstream ohandle(dir / "file.jpg");
        constexpr std::array<unsigned char, 4> stuff { 0xff, 0xd8, 0xff, 0xe1 };
        ohandle.write(reinterpret_cast<const char*>(stuff.data()), stuff.size());
    }
    test_validate(dir);
}

TEST(ImageFile, JpegError) {
    auto dir = define_test_path("image_file");

    {
        initialize_directory(dir);
        quick_text_write(
            dir / "OBJECT",
            "{ \"type\": \"image_file\", \"image_file\": { \"version\": \"1.0\", \"format\": \"JPEG\" } }"
        );
        quick_text_write(dir / "file.jpg", "chino-chan");
    }
    expect_validation_error(dir, "JPEG file");
}

/***********************************************/

TEST(ImageFile, GifOkay) {
    auto dir = define_test_path("image_file");

    {
        initialize_directory(dir);
        quick_text_write(
            dir / "OBJECT",
            "{ \"type\": \"image_file\", \"image_file\": { \"version\": \"1.0\", \"format\": \"GIF\" } }"
        );
        std::ofstream ohandle(dir / "file.gif");
        constexpr std::array<unsigned char, 4> stuff { 0x47, 0x49, 0x46, 0x38 };
        ohandle.write(reinterpret_cast<const char*>(stuff.data()), stuff.size());
    }
    test_validate(dir);
}

TEST(ImageFile, GifError) {
    auto dir = define_test_path("image_file");

    {
        initialize_directory(dir);
        quick_text_write(
            dir / "OBJECT",
            "{ \"type\": \"image_file\", \"image_file\": { \"version\": \"1.0\", \"format\": \"GIF\" } }"
        );
        quick_text_write(dir / "file.gif", "chino-chan");
    }
    expect_validation_error(dir, "GIF file");
}

/***********************************************/

TEST(ImageFile, WebpOkay) {
    auto dir = define_test_path("image_file");

    {
        initialize_directory(dir);
        quick_text_write(
            dir / "OBJECT",
            "{ \"type\": \"image_file\", \"image_file\": { \"version\": \"1.0\", \"format\": \"WEBP\" } }"
        );
        std::ofstream ohandle(dir / "file.webp");
        constexpr std::array<unsigned char, 12> stuff { 0x52, 0x49, 0x46, 0x46, 0x0, 0x0, 0x0, 0x0, 0x57, 0x45, 0x42, 0x50 };
        ohandle.write(reinterpret_cast<const char*>(stuff.data()), stuff.size());
    }
    test_validate(dir);
}

TEST(ImageFile, WebpError) {
    auto dir = define_test_path("image_file");

    {
        initialize_directory(dir);
        quick_text_write(
            dir / "OBJECT",
            "{ \"type\": \"image_file\", \"image_file\": { \"version\": \"1.0\", \"format\": \"WEBP\" } }"
        );
        quick_text_write(dir / "file.webp", "kirima-syaro");
    }
    expect_validation_error(dir, "WEBP file");

    // First 4 bytes are okay but last 4 bytes don't validate.
    {
        initialize_directory(dir);
        quick_text_write(
            dir / "OBJECT",
            "{ \"type\": \"image_file\", \"image_file\": { \"version\": \"1.0\", \"format\": \"WEBP\" } }"
        );
        std::ofstream ohandle(dir / "file.webp");
        constexpr std::array<unsigned char, 12> stuff { 0x52, 0x49, 0x46, 0x46, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0 };
        ohandle.write(reinterpret_cast<const char*>(stuff.data()), stuff.size());
    }
    expect_validation_error(dir, "WEBP file");
}

/***********************************************/

TEST(ImageFile, VersionError) {
    auto dir = define_test_path("image_file");

    {
        initialize_directory_simple(dir, "image_file", "2.0");
    }
    expect_validation_error(dir, "unsupported version");
}

TEST(ImageFile, FormatError) {
    auto dir = define_test_path("image_file");

    {
        initialize_directory(dir);
        quick_text_write(
            dir / "OBJECT",
            "{ \"type\": \"image_file\", \"image_file\": { \"version\": \"1.0\", \"format\": \"FOOBAR\" } }"
        );
    }
    expect_validation_error(dir, "unsupported format");
}

/***********************************************/

static void mock_png(const std::filesystem::path& dir) {
    initialize_directory(dir);
    quick_text_write(
        dir / "OBJECT",
        "{ \"type\": \"image_file\", \"image_file\": { \"version\": \"1.0\", \"format\": \"PNG\" } }"
    );
    std::ofstream ohandle(dir / "file.png");
    constexpr std::array<unsigned char, 8> stuff { 137, 80, 78, 71, 13, 10, 26, 10 };
    ohandle.write(reinterpret_cast<const char*>(stuff.data()), stuff.size());
}

TEST(ImageFile, StrictOkay) {
    auto dir = define_test_path("image_file");

    {
        mock_png(dir);
    }

    takane::Options opt;
    opt.image_file_strict_check = [&](const std::filesystem::path&, const takane::ObjectMetadata&, const takane::Options&) {};
    test_validate(dir);
}

TEST(ImageFile, StrictError) {
    auto dir = define_test_path("image_file");

    {
        mock_png(dir);
    }

    takane::Options opt;
    opt.image_file_strict_check = [&](const std::filesystem::path&, const takane::ObjectMetadata&, const takane::Options&) { throw std::runtime_error("FOOBAR"); };
    expect_validation_error(dir, "FOOBAR", opt);
}
