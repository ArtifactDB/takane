#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "takane/utils_json.hpp"
#include "takane/utils_public.hpp"
#include "utils.h"

TEST(ParseJsonFile, Basic) {
    auto dir = define_test_path("utils_json");
    initialize_directory(dir);
    auto path = dir / "FOO.json";

    {
        std::ofstream ostream(path);
        ostream << "{ \"abc\": true }";
    }

    auto parsed = takane::parse_json_file(path);
    EXPECT_EQ(parsed->type(), millijson::OBJECT);
}

TEST(ExtractJsonObject, Basic) {
    auto dir = define_test_path("utils_json");
    initialize_directory(dir);
    auto path = dir / "OBJECT";

    {
        std::ofstream ostream(path);
        ostream << "{ \"type\": \"foo\", \"foo\": { \"a\": 2, \"bc\": false }, \"bar\": 2 }";
    }

    auto parsed = takane::read_object_metadata(dir);
    const auto& res = takane::extract_json_object(parsed.other, "foo");
    EXPECT_EQ(res.size(), 2);
    EXPECT_TRUE(res.find("a") != res.end());
    EXPECT_TRUE(res.find("bc") != res.end());

    expect_error(
        "not present",
        [&]() -> void {
            takane::extract_json_object(parsed.other, "whee");
        }
    );

    expect_error(
        "JSON object",
        [&]() -> void {
            takane::extract_json_object(parsed.other, "bar");
        }
    );
}

TEST(ExtractJsonString, Basic) {
    auto dir = define_test_path("utils_json");
    initialize_directory(dir);
    auto path = dir / "OBJECT";

    {
        std::ofstream ostream(path);
        ostream << "{ \"type\": \"foo\", \"foo\": \"abc\", \"bar\": 2 }";
    }

    auto parsed = takane::read_object_metadata(dir);
    EXPECT_EQ(takane::extract_json_string(parsed.other, "foo"), "abc");

    expect_error(
        "not present",
        [&]() -> void {
            takane::extract_json_string(parsed.other, "whee");
        }
    );

    expect_error(
        "JSON string",
        [&]() -> void {
            takane::extract_json_string(parsed.other, "bar");
        }
    );
}

TEST(ExtractJsonVersionString, Basic) {
    auto dir = define_test_path("utils_json");
    initialize_directory(dir);
    auto path = dir / "OBJECT";

    {
        std::ofstream output(path);
        output << "{ \"type\": \"foobar\", \"foobar\": { \"version\": \"2.1.0\" } }";
    }
    {
        auto parsed = takane::read_object_metadata(dir);
        auto extracted = takane::extract_json_object(parsed.other, "foobar");
        EXPECT_EQ(takane::extract_json_version_string(extracted), "2.1.0");
    }

    // Rethrows an error correctly.
    {
        std::ofstream output(path);
        output << "{ \"type\": \"foobar\", \"foobar\": { \"version\": 2 } }";
    }
    {
        auto parsed = takane::read_object_metadata(dir);
        auto extracted = takane::extract_json_object(parsed.other, "foobar");
        expect_error(
            "JSON string",
            [&]() -> void {
                takane::extract_json_version_string(extracted);
            }
        );
    }
}
