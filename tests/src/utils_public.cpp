#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "takane/utils_public.hpp"
#include "utils.h"

#include <string>
#include <filesystem>

static void expect_read_object_error(const std::filesystem::path& dir, const std::string& msg) {
    expect_error(
        msg,
        [&]() -> void {
            takane::read_object_metadata(dir);
        }
    );
}

TEST(ReadObjectMetadata, Basic) {
    auto dir = define_test_path("utils_public");

    initialize_directory(dir);
    auto objpath = (dir / "OBJECT").string();

    quick_text_write(objpath, "{ \"type\": \"foo_bar 2\" }");
    EXPECT_EQ(takane::read_object_metadata(dir).type, "foo_bar 2");

    // Works across multiple lines.
    quick_text_write(objpath, "{ \"type\": \"baz-stuff\", \n \"foobar\": \"whee\" }\n");
    {
        auto meta = takane::read_object_metadata(dir);
        EXPECT_EQ(meta.type, "baz-stuff");
        EXPECT_EQ(meta.other.size(), 1);
        EXPECT_TRUE(meta.other.find("foobar") != meta.other.end());
    }

    quick_text_write(objpath, "{ \"type\": \"baz-stuff\", \n \"foobar\": \"whee\", \"superblah\"\n:\n2 }\n");
    {
        auto meta = takane::read_object_metadata(dir);
        EXPECT_EQ(meta.type, "baz-stuff");
        EXPECT_EQ(meta.other.size(), 2);
        EXPECT_TRUE(meta.other.find("foobar") != meta.other.end());
        EXPECT_TRUE(meta.other.find("superblah") != meta.other.end());
    }
}

TEST(ReadObjectMetadata, Error) {
    auto dir = define_test_path("utils_public");

    initialize_directory(dir);
    auto objpath = (dir / "OBJECT").string();

    quick_text_write(objpath, "[]");
    expect_read_object_error(dir, "JSON object");

    quick_text_write(objpath, "{}");
    expect_read_object_error(dir, "type");

    quick_text_write(objpath, "{ \"type\": 2 }");
    expect_read_object_error(dir, "string");
}
