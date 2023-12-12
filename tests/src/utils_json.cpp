#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "takane/takane.hpp"
#include "utils.h"

struct ExtractJsonTest : public::testing::Test {
    ExtractJsonTest() {
        dir = "TEST_json";
    }

    std::filesystem::path dir;

    template<typename ... Args_>
    void expect_object_error(const std::string& msg, Args_&& ... args) {
        auto parsed = takane::read_object_metadata(dir);
        EXPECT_ANY_THROW({
            try {
                takane::internal_json::extract_typed_object_from_metadata(parsed.other, std::forward<Args_>(args)...);
            } catch (std::exception& e) {
                EXPECT_THAT(e.what(), ::testing::HasSubstr(msg));
                throw;
            }
        });
    }

    template<typename ... Args_>
    void extract_version_error(const std::string& msg, Args_&& ... args) {
        auto parsed = takane::read_object_metadata(dir);
        EXPECT_ANY_THROW({
            try {
                takane::internal_json::extract_version_for_type(parsed.other, std::forward<Args_>(args)...);
            } catch (std::exception& e) {
                EXPECT_THAT(e.what(), ::testing::HasSubstr(msg));
                throw;
            }
        });
    }
};

TEST_F(ExtractJsonTest, Object) {
    initialize_directory(dir);

    auto objpath = dir / "OBJECT";
    {
        std::ofstream output(objpath);
        output << "{ \"type\": \"foob\", \"foobar\": \"1,2,3,4\" }";
    }
    expect_object_error("not present", "whee");
    expect_object_error("JSON object", "foobar");

    {
        std::ofstream output(objpath);
        output << "{ \"type\": \"foob\", \"foobar\": { \"foo\": 1, \"bar\": 2 } }";
    }
    auto parsed = takane::read_object_metadata(dir);
    auto extracted = takane::internal_json::extract_typed_object_from_metadata(parsed.other, "foobar");
    EXPECT_EQ(extracted.size(), 2);
}

TEST_F(ExtractJsonTest, String) {
    initialize_directory(dir);

    auto objpath = dir / "OBJECT";
    {
        std::ofstream output(objpath);
        output << "{ \"type\": \"foob\", \"foobar\": \"1,2,3,4\" }";
    }
    extract_version_error("not present", "whee");
    extract_version_error("JSON object", "foobar");

    {
        std::ofstream output(objpath);
        output << "{ \"type\": \"foob\", \"foobar\": { \"foo\": 1, \"bar\": 2 } }";
    }
    extract_version_error("not present", "foobar");

    {
        std::ofstream output(objpath);
        output << "{ \"type\": \"foob\", \"foobar\": { \"version\": 1, \"bar\": 2 } }";
    }
    extract_version_error("JSON string", "foobar");

    {
        std::ofstream output(objpath);
        output << "{ \"type\": \"foob\", \"foobar\": { \"version\": \"1.2\", \"bar\": 2 } }";
    }
    auto parsed = takane::read_object_metadata(dir);
    EXPECT_EQ(takane::internal_json::extract_version_for_type(parsed.other, "foobar"), "1.2");
}
