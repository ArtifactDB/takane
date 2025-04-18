#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "utils.h"
#include "takane/takane.hpp"

#include <filesystem>

void test_validate(const std::filesystem::path& dir) {
    takane::validate(dir);
}

void test_validate(const std::filesystem::path& dir, takane::Options& opts) {
    takane::validate(dir, opts);
}

size_t test_height(const std::filesystem::path& dir) {
    return takane::height(dir);
}

size_t test_height(const std::filesystem::path& dir, takane::Options& opts) {
    return takane::height(dir, opts);
}

std::vector<size_t> test_dimensions(const std::filesystem::path& dir) {
    return takane::dimensions(dir);
}

std::vector<size_t> test_dimensions(const std::filesystem::path& dir, takane::Options& opts) {
    return takane::dimensions(dir, opts);
}

// Just testing that our JSON dumping code works as expected.
TEST(JsonDump, BasicDumps) {
    auto test = new millijson::Object({});
    std::shared_ptr<millijson::Base> store(test);
    test->value()["number"] = std::shared_ptr<millijson::Base>(new millijson::Number(1));
    test->value()["string"] = std::shared_ptr<millijson::Base>(new millijson::String("foo"));

    auto atest = new millijson::Array({});
    test->value()["array"] = std::shared_ptr<millijson::Base>(atest);
    atest->value().emplace_back(new millijson::Boolean(true));
    atest->value().emplace_back(new millijson::Nothing);
    atest->value().emplace_back(new millijson::Boolean(false));

    initialize_directory("TEST_json");
    json_utils::dump(store.get(), "TEST_json/OBJECT");
    std::ifstream input("TEST_json/OBJECT");
    std::stringstream stream;
    stream << input.rdbuf();

    EXPECT_EQ(stream.str(), "{\"array\": [true, null, false], \"number\": 1, \"string\": \"foo\"}");
}
