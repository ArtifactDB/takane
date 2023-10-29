#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "takane/string_factor.hpp"

#include <fstream>
#include <string>

static void validate(const std::string& buffer, size_t length) {
    std::string path = "TEST-string_factor.csv";
    {
        std::ofstream ohandle(path);
        ohandle << buffer;
    }
    takane::string_factor::LevelParameters params;
    params.num_levels = length;
    takane::string_factor::validate_levels(path.c_str(), params);
}

template<typename ... Args_>
static void expect_error(const std::string& msg, const std::string& buffer, Args_&& ... args) {
    EXPECT_ANY_THROW({
        try {
            validate(buffer, std::forward<Args_>(args)...);
        } catch (std::exception& e) {
            EXPECT_THAT(e.what(), ::testing::HasSubstr(msg));
            throw;
        }
    });
}

TEST(StringFactor, LevelBasics) {
    std::string buffer = "\"values\"\n";
    buffer += "\"foo\"\n";
    buffer += "\"bar\"\n";
    buffer += "\"whee\"\n";
    buffer += "\"stuff\"\n";
    ::validate(buffer, 4);
    expect_error("number of records", buffer, 10);

    buffer = "\"values\"\n";
    buffer += "-1\n";
    expect_error("types", buffer, 1);

    buffer = "\"whee\"\n";
    buffer += "\"a\"\n";
    expect_error("header name", buffer, 1);

    buffer = "\"values\"\n";
    buffer += "\"a\"\n";
    buffer += "\"a\"\n";
    expect_error("duplicated level", buffer, 2);
}
