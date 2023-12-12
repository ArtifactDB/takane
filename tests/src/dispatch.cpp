#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "takane/takane.hpp"
#include "utils.h"

#include <filesystem>

TEST(GenericDispatch, Validate) {
    std::filesystem::path dir = "TEST_dispatcher";
    initialize_directory_simple(dir, "foobar", "1.0");
    expect_validation_error(dir, "no registered 'validate' function");

    takane::validate_registry["foobar"] = [](const std::filesystem::path&, const takane::ObjectMetadata&, const takane::Options&) -> void {};
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
    initialize_directory_simple(dir, "foobar", "1.0");
    expect_height_error(dir, "no registered 'height' function");

    takane::height_registry["foobar"] = [](const std::filesystem::path&, const takane::ObjectMetadata&, const takane::Options&) -> size_t { return 11; };
    EXPECT_EQ(takane::height(dir), 11);
    takane::height_registry.erase("foobar");
}
