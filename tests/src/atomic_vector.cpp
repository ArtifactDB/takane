#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "takane/atomic_vector.hpp"

#include <fstream>
#include <string>

static void validate(const std::string& buffer, size_t length, takane::atomic_vector::Type type, bool has_names) {
    std::string path = "TEST-atomic_vector.csv";
    {
        std::ofstream ohandle(path);
        ohandle << buffer;
    }
    takane::atomic_vector::validate(path.c_str(), length, type, has_names);
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

TEST(AtomicVector, Basics) {
    std::string buffer = "\"names\",\"values\"\n";
    buffer += "\"foo\",\"whee\"\n";
    buffer += "\"bar\",\"bob\"\n";
    buffer += "\"whee\",\"stuff\"\n";
    ::validate(buffer, 3, takane::atomic_vector::Type::STRING, true);

    expect_error("number of header names", buffer, 3, takane::atomic_vector::Type::STRING, false);
    expect_error("number of records", buffer, 10, takane::atomic_vector::Type::STRING, true);

    buffer = "\"names\",\"blah\"\n";
    buffer += "\"foo\",\"whee\"\n";
    buffer += "\"bar\",\"bob\"\n";
    buffer += "\"whee\",\"stuff\"\n";
    expect_error("should be named 'values'", buffer, 3, takane::atomic_vector::Type::STRING, true);

    buffer = "\"values\"\n";
    buffer += "\"foo\"\n";
    buffer += "\"bar\"\n";
    buffer += "\"whee\"\n";
    ::validate(buffer, 3, takane::atomic_vector::Type::STRING, false);
}

TEST(AtomicVector, Types) {
    std::string buffer = "\"names\",\"values\"\n";
    buffer += "\"foo\",1.2\n";
    buffer += "\"bar\",3.4\n";
    buffer += "\"whee\",5.6\n";
    ::validate(buffer, 3, takane::atomic_vector::Type::NUMBER, true);
    expect_error("not an integer", buffer, 3, takane::atomic_vector::Type::INTEGER, true);

    buffer = "\"values\"\n";
    buffer += "true\n";
    buffer += "false\n";
    buffer += "true\n";
    ::validate(buffer, 3, takane::atomic_vector::Type::BOOLEAN, false);

    buffer = "\"values\"\n";
    buffer += "23231\n";
    buffer += "-112312\n";
    buffer += "81\n";
    ::validate(buffer, 3, takane::atomic_vector::Type::INTEGER, false);
}
