#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "takane/takane.hpp"
#include "utils.h"

#include <filesystem>

TEST(GenericDispatch, Validate) {
    takane::Options opts;

    std::filesystem::path dir = "TEST_dispatcher";
    initialize_directory_simple(dir, "foobar", "1.0");
    expect_validation_error(dir, "no registered 'validate' function", opts);

    opts.custom_validate["foobar"] = [](const std::filesystem::path&, const takane::ObjectMetadata&, const takane::Options&) -> void {};
    test_validate(dir, opts);
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
    takane::Options opts;

    std::filesystem::path dir = "TEST_dispatcher";
    initialize_directory_simple(dir, "foobar", "1.0");
    expect_height_error(dir, "no registered 'height' function", opts);

    opts.custom_height["foobar"] = [](const std::filesystem::path&, const takane::ObjectMetadata&, const takane::Options&) -> size_t { return 11; };
    EXPECT_EQ(test_height(dir, opts), 11);
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
    takane::Options opts;

    std::filesystem::path dir = "TEST_dispatcher";
    initialize_directory_simple(dir, "foobar", "1.0");
    expect_dimensions_error(dir, "no registered 'dimensions' function", opts);

    std::vector<size_t> expected { 11, 20 };
    opts.custom_dimensions["foobar"] = [&](const std::filesystem::path&, const takane::ObjectMetadata&, const takane::Options&) -> std::vector<size_t> { return expected; };
    EXPECT_EQ(test_dimensions(dir, opts), expected);
}

TEST(GenericDispatch, SatisfiesInterface) {
    takane::Options opts;
    EXPECT_TRUE(takane::satisfies_interface("summarized_experiment", "SUMMARIZED_EXPERIMENT", opts));
    EXPECT_TRUE(takane::satisfies_interface("single_cell_experiment", "SUMMARIZED_EXPERIMENT", opts));

    EXPECT_FALSE(takane::satisfies_interface("foo", "FOO", opts));
    opts.custom_satisfies_interface["FOO"] = std::unordered_set<std::string>{ "foo" };
    EXPECT_TRUE(takane::satisfies_interface("foo", "FOO", opts));
}

TEST(GenericDispatch, DerivedFrom) {
    takane::Options opts;

    EXPECT_TRUE(takane::derived_from("summarized_experiment", "summarized_experiment", opts));
    EXPECT_TRUE(takane::derived_from("ranged_summarized_experiment", "summarized_experiment", opts));
    EXPECT_TRUE(takane::derived_from("single_cell_experiment", "summarized_experiment", opts));
    EXPECT_FALSE(takane::derived_from("vcf_experiment", "summarized_experiment", opts));

    EXPECT_FALSE(takane::derived_from("foo", "FOO", opts));
    opts.custom_derived_from["FOO"] = std::unordered_set<std::string>{ "foo" };
    EXPECT_TRUE(takane::derived_from("foo", "FOO", opts));
}
