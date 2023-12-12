#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "takane/takane.hpp"
#include "utils.h"

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

    auto objpath = dir / "OBJECT";
    {
        std::ofstream output(objpath);
        output << "{ \"type\": \"foo_bar 2\" }";
    }
    EXPECT_EQ(takane::read_object_metadata(dir).type, "foo_bar 2");

    // Works across multiple lines.
    {
        std::ofstream output(objpath);
        output << "{ \"type\": \"baz-stuff\", \n \"foobar\": \"whee\" }\n";
    }
    auto meta = takane::read_object_metadata(dir);
    EXPECT_EQ(meta.type, "baz-stuff");
    EXPECT_EQ(meta.other.size(), 1);
}

TEST_F(ReadObjectTest, Fails) {
    initialize_directory(dir);

    auto objpath = dir / "OBJECT";
    {
        std::ofstream output(objpath);
        output << "[]";
    }
    expect_error("JSON object");

    {
        std::ofstream output(objpath);
        output << "{}";
    }
    expect_error("type");

    {
        std::ofstream output(objpath);
        output << "{ \"type\": 2 }";
    }
    expect_error("string");
}
