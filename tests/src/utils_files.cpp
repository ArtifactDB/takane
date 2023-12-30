#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "takane/utils_files.hpp"

#include "utils.h"

struct FileSignatureTest : public::testing::Test {
    FileSignatureTest() {
        dir = "TEST_files";
    }

    std::filesystem::path dir;

    template<typename ... Args_>
    static void expect_error_check(const std::string& msg, Args_&& ... args) {
        EXPECT_ANY_THROW({
            try {
                takane::internal_files::check_signature(std::forward<Args_>(args)...);
            } catch (std::exception& e) {
                EXPECT_THAT(e.what(), ::testing::HasSubstr(msg));
                throw;
            }
        });
    }

    template<typename ... Args_>
    static void expect_error_extract(const std::string& msg, Args_&& ... args) {
        EXPECT_ANY_THROW({
            try {
                takane::internal_files::extract_signature(std::forward<Args_>(args)...);
            } catch (std::exception& e) {
                EXPECT_THAT(e.what(), ::testing::HasSubstr(msg));
                throw;
            }
        });
    }
};

TEST_F(FileSignatureTest, Character) {
    initialize_directory(dir);
    auto path = dir / "foo.png";

    {
        std::ofstream handle(path);
        handle << "";
    }
    expect_error_check("incomplete ASD file signature", path, "FOOBAR", 6, "ASD");

    {
        std::ofstream handle(path);
        handle << "FOObar";
    }
    expect_error_check("incorrect ASD file signature", path, "FOOBAR", 6, "ASD");

    {
        std::ofstream handle(path);
        handle << "FOOBAR";
    }
    takane::internal_files::check_signature(path, "FOOBAR", 6, "ASD");

    // Works with non-ASCII characters.
    {
        std::ofstream handle(path);
        handle << "FOO\1BAR\2asdasd\3asd\n";
    }
    takane::internal_files::check_signature(path, "FOO\1BAR\2", 8, "ASD");
}

TEST_F(FileSignatureTest, Unsigned) {
    initialize_directory(dir);
    auto path = dir / "foo.bam";
    const unsigned char foo[4] = { 0x4a, 0x55, 0xf2, 0x90 };

    {
        std::ofstream handle(path);
        handle << "";
    }
    expect_error_check("incomplete ASD file signature", path, foo, 4, "ASD");

    {
        std::ofstream handle(path);
        handle << "FOObar";
    }
    expect_error_check("incorrect ASD file signature", path, foo, 4, "ASD");

    {
        byteme::RawFileWriter writer(path);
        writer.write(foo, 4);
    }
    takane::internal_files::check_signature(path, foo, 4, "ASD");
}

TEST_F(FileSignatureTest, Extraction) {
    initialize_directory(dir);
    auto path = dir / "foo.bam";
    unsigned char buffer[4];

    {
        std::ofstream handle(path);
        handle << "";
    }
    expect_error_extract("too small", path, buffer, 4);

    {
        std::ofstream handle(path);
        handle << "FOObar";
    }
    takane::internal_files::extract_signature(path, buffer, 4);
    auto cbuffer = reinterpret_cast<char*>(buffer);
    EXPECT_EQ(cbuffer[0], 'F');
    EXPECT_EQ(cbuffer[1], 'O');
    EXPECT_EQ(cbuffer[2], 'O');
    EXPECT_EQ(cbuffer[3], 'b');
}
