#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "takane/_satisfies_interface.hpp"

#include <filesystem>

TEST(SatisfiesInterface, Basic) {
    takane::Options opts;
    EXPECT_TRUE(takane::satisfies_interface("summarized_experiment", "SUMMARIZED_EXPERIMENT", opts));
    EXPECT_TRUE(takane::satisfies_interface("single_cell_experiment", "SUMMARIZED_EXPERIMENT", opts));

    EXPECT_FALSE(takane::satisfies_interface("foo", "FOO", opts));
    opts.custom_satisfies_interface["FOO"] = std::unordered_set<std::string>{ "foo" };
    EXPECT_TRUE(takane::satisfies_interface("foo", "FOO", opts));
}
