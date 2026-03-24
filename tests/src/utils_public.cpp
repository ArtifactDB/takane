#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "takane/utils_public.hpp"
#include "utils.h"

#include <string>
#include <filesystem>

struct ReadObjectTest : public::testing::Test {
    ReadObjectTest() {
        dir = "TEST_readobj";
    }

    std::filesystem::path dir;

    void expect_error(const std::string& msg) {
        EXPECT_ANY_THROW({
            try {
                takane::read_object_metadata(dir);
            } catch (std::exception& e) {
                EXPECT_THAT(e.what(), ::testing::HasSubstr(msg));
                throw;
            }
        });
    }
};

TEST_F(ReadObjectTest, Basic) {
    initialize_directory(dir);
    auto objpath = (dir / "OBJECT").string();

    quick_text_write(objpath, "{ \"type\": \"foo_bar 2\" }");
    EXPECT_EQ(takane::read_object_metadata(dir).type, "foo_bar 2");

    // Works across multiple lines.
    quick_text_write(objpath, "{ \"type\": \"baz-stuff\", \n \"foobar\": \"whee\" }\n");
    auto meta = takane::read_object_metadata(dir);
    EXPECT_EQ(meta.type, "baz-stuff");
    EXPECT_EQ(meta.other.size(), 1);
}

TEST_F(ReadObjectTest, Fails) {
    initialize_directory(dir);
    auto objpath = (dir / "OBJECT").string();

    quick_text_write(objpath, "[]");
    expect_error("JSON object");

    quick_text_write(objpath, "{}");
    expect_error("type");

    quick_text_write(objpath, "{ \"type\": 2 }");
    expect_error("string");
}
