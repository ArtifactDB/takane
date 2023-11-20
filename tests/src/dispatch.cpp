#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "takane/takane.hpp"
#include "utils.h"

#include <filesystem>

TEST(ReadObjectTest, Basic) {
    std::filesystem::path dir = "TEST_readObj";
    if (std::filesystem::exists(dir)) {
        std::filesystem::remove_all(dir);
    }
    std::filesystem::create_directory(dir);

    // Works with a trailing newline.
    auto objpath = dir / "OBJECT";
    {
        std::ofstream output(objpath);
        output << "foo_bar 2\n";
    }
    EXPECT_EQ(takane::read_object_type(dir), "foo_bar 2");

    {
        std::ofstream output(objpath);
        output << "baz-stuff";
    }
    EXPECT_EQ(takane::read_object_type(dir), "baz-stuff");
}

TEST(GenericDispatch, Validate) {
    std::filesystem::path dir = "TEST_dispatcher";
    initialize_directory(dir, "foobar");
    expect_validation_error(dir, "no registered 'validate' function");

    takane::validate_registry["foobar"] = [](const std::filesystem::path&, const takane::Options&) -> void {};
    takane::validate(dir);
    takane::validate_registry.erase("foobar");
}

template<typename ... Args_>
void expect_height_error(const std::filesystem::path& dir, const std::string& msg, Args_&& ... args) {
    EXPECT_ANY_THROW({
        try {
            takane::height(dir, std::forward<Args_>(args)...);
        } catch (std::exception& e) {
            EXPECT_THAT(e.what(), ::testing::HasSubstr(msg));
            throw;
        }
    });
}

TEST(GenericDispatch, Height) {
    std::filesystem::path dir = "TEST_dispatcher";
    initialize_directory(dir, "foobar");
    expect_height_error(dir, "no registered 'height' function");

    takane::height_registry["foobar"] = [](const std::filesystem::path&, const takane::Options&) -> size_t { return 11; };
    EXPECT_EQ(takane::height(dir), 11);
    takane::height_registry.erase("foobar");
}
