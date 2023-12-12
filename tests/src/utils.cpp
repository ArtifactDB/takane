#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "utils.h"

// Just testing that our JSON dumping code works as expected.
TEST(JsonDump, BasicDumps) {
    auto test = new millijson::Object;
    std::shared_ptr<millijson::Base> store(test);
    test->add("number", std::shared_ptr<millijson::Base>(new millijson::Number(1)));
    test->add("string", std::shared_ptr<millijson::Base>(new millijson::String("foo")));

    auto atest = new millijson::Array;
    test->add("array", std::shared_ptr<millijson::Base>(atest));
    atest->add(std::shared_ptr<millijson::Base>(new millijson::Boolean(true)));
    atest->add(std::shared_ptr<millijson::Base>(new millijson::Nothing));
    atest->add(std::shared_ptr<millijson::Base>(new millijson::Boolean(false)));

    initialize_directory("TEST_json");
    json_utils::dump(store.get(), "TEST_json/OBJECT");
    std::ifstream input("TEST_json/OBJECT");
    std::stringstream stream;
    stream << input.rdbuf();

    EXPECT_EQ(stream.str(), "{\"array\": [true, null, false], \"number\": 1, \"string\": \"foo\"}");
}
