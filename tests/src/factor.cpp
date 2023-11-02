#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "takane/factor.hpp"

#include <fstream>
#include <string>

static void validate(const std::string& buffer, size_t length, size_t num_levels, bool has_names) {
    std::string path = "TEST-factor.csv";
    {
        std::ofstream ohandle(path);
        ohandle << buffer;
    }

    takane::factor::Parameters params;
    params.length = length;
    params.num_levels = num_levels;
    params.has_names = has_names;

    takane::factor::validate(path.c_str(), params);
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

TEST(Factor, Basics) {
    std::string buffer = "\"names\",\"values\"\n";
    buffer += "\"foo\",0\n";
    buffer += "\"bar\",1\n";
    buffer += "\"whee\",2\n";
    buffer += "\"stuff\",NA\n";
    ::validate(buffer, 4, 3, true);

    expect_error("number of header names", buffer, 4, 3, false);
    expect_error("less than the number of levels", buffer, 4, 2, true);
    expect_error("number of records", buffer, 10, 3, true);
    expect_error("number of levels must fit", buffer, 4, 3000000000, true);

    buffer = "\"values\"\n";
    buffer += "-1\n";
    expect_error("should be non-negative", buffer, 1, 3, false);

    buffer = "\"values\"\n";
    buffer += "1.5\n";
    expect_error("not an integer", buffer, 1, 3, false);
}
