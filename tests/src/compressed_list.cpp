#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "takane/compressed_list.hpp"
#include "utils.h"

#include <fstream>
#include <string>

static takane::CsvContents validate(const std::string& buffer, size_t length, size_t concatenated, bool has_names, takane::CsvFieldCreator* creator = NULL) {
    std::string path = "TEST-compressed_list.csv";
    {
        std::ofstream ohandle(path);
        ohandle << buffer;
    }

    takane::compressed_list::Parameters params;
    params.length = length;
    params.concatenated = concatenated;
    params.has_names = has_names;

    return takane::compressed_list::validate(path.c_str(), params, creator);
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

TEST(CompressedList, Basics) {
    std::string buffer = "\"names\",\"number\"\n";
    buffer += "\"foo\",2\n";
    buffer += "\"bar\",5\n";
    buffer += "\"whee\",5\n";
    ::validate(buffer, 3, 12, true);

    expect_error("number of header names", buffer, 3, 12, false);
    expect_error("number of records", buffer, 10, 12, true);
    expect_error("concatenated total", buffer, 3, 9, true);

    FilledFieldCreator filled;
    auto output = ::validate(buffer, 3, 12, true, &filled);
    EXPECT_EQ(output.fields.size(), 2);
    EXPECT_EQ(output.fields[0]->type(), comservatory::Type::STRING);
    EXPECT_EQ(output.fields[1]->type(), comservatory::Type::NUMBER);

    buffer = "\"names\",\"whee\"\n";
    buffer += "\"foo\",2\n";
    buffer += "\"bar\",5\n";
    buffer += "\"whee\",2\n";
    expect_error("should be named 'number'", buffer, 3, 9, true);

    buffer = "\"number\"\n";
    buffer += "-2\n";
    buffer += "5\n";
    buffer += "2\n";
    expect_error("should not be negative", buffer, 3, 9, false);

    buffer = "\"number\"\n";
    buffer += "1.5\n";
    expect_error("not an integer", buffer, 1, 2, false);

    buffer = "\"number\"\n";
    buffer += "15000000000\n";
    expect_error("does not fit", buffer, 1, 2, false);

    buffer = "\"number\"\n";
    buffer += "NA\n";
    expect_error("should not be missing", buffer, 1, 9, false);
}
