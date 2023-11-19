#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "takane/takane.hpp"
#include "utils.h"

#include <filesystem>

TEST(GenericDispatch, Validate) {
    std::filesystem::path dir = "TEST_dispatcher";
    initialize_directory(dir, "foobar");
    expect_validation_error(dir, "no registered validation function");

    takane::validate_override = [](const std::filesystem::path&, const std::string&, const takane::Options&) -> bool { return true; };
    takane::validate(dir);

    takane::validate_override = [](const std::filesystem::path&, const std::string&, const takane::Options&) -> bool { return false; };
    expect_validation_error(dir, "no registered validation function");
    takane::validate_override = nullptr;

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
    expect_height_error(dir, "no registered height function");

    takane::height_override = [](const std::filesystem::path&, const std::string&, const takane::Options&) -> std::pair<bool, size_t> { return std::make_pair(true, 99); };
    EXPECT_EQ(takane::height(dir), 99);

    takane::height_override = [](const std::filesystem::path&, const std::string&, const takane::Options&) -> std::pair<bool, size_t> { return std::make_pair(false, 0); };
    expect_height_error(dir, "no registered height function");
    takane::height_override = nullptr;

    takane::height_registry["foobar"] = [](const std::filesystem::path&, const takane::Options&) -> size_t { return 11; };
    EXPECT_EQ(takane::height(dir), 11);
    takane::height_registry.erase("foobar");
}
