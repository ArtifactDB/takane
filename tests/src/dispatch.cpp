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
    test_validate(dir);
    takane::validate_registry.erase("foobar");
}

template<typename ... Args_>
void expect_height_error(const std::filesystem::path& dir, const std::string& msg, Args_&& ... args) {
    EXPECT_ANY_THROW({
        try {
            test_height(dir, std::forward<Args_>(args)...);
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
    EXPECT_EQ(test_height(dir), 11);
    takane::height_registry.erase("foobar");
}

template<typename ... Args_>
void expect_dimensions_error(const std::filesystem::path& dir, const std::string& msg, Args_&& ... args) {
    EXPECT_ANY_THROW({
        try {
            test_dimensions(dir, std::forward<Args_>(args)...);
        } catch (std::exception& e) {
            EXPECT_THAT(e.what(), ::testing::HasSubstr(msg));
            throw;
        }
    });
}

TEST(GenericDispatch, Dimensions) {
    std::filesystem::path dir = "TEST_dispatcher";
    initialize_directory_simple(dir, "foobar", "1.0");
    expect_dimensions_error(dir, "no registered 'dimensions' function");

    std::vector<size_t> expected { 11, 20 };
    takane::dimensions_registry["foobar"] = [&](const std::filesystem::path&, const takane::ObjectMetadata&, const takane::Options&) -> std::vector<size_t> { return expected; };
    EXPECT_EQ(test_dimensions(dir), expected);
    takane::dimensions_registry.erase("foobar");
}

TEST(GenericDispatch, SatisfiesInterface) {
    EXPECT_FALSE(takane::satisfies_interface("foo", "FOO"));
    takane::satisfies_interface_registry["FOO"] = std::unordered_set<std::string>{ "foo" };
    EXPECT_TRUE(takane::satisfies_interface("foo", "FOO"));
    takane::satisfies_interface_registry.erase("FOO");

    EXPECT_TRUE(takane::satisfies_interface("summarized_experiment", "SUMMARIZED_EXPERIMENT"));
    EXPECT_TRUE(takane::satisfies_interface("single_cell_experiment", "SUMMARIZED_EXPERIMENT"));
}

TEST(GenericDispatch, DerivedFrom) {
    EXPECT_TRUE(takane::derived_from("summarized_experiment", "summarized_experiment"));
    EXPECT_TRUE(takane::derived_from("ranged_summarized_experiment", "summarized_experiment"));
    EXPECT_TRUE(takane::derived_from("single_cell_experiment", "summarized_experiment"));
    EXPECT_FALSE(takane::derived_from("vcf_experiment", "summarized_experiment"));

    takane::derived_from_registry["FOO"] = std::unordered_set<std::string>{ "foo" };
    EXPECT_TRUE(takane::derived_from("foo", "FOO"));
    takane::derived_from_registry.erase("FOO");
}
