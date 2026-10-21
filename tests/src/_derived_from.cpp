#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "takane/_derived_from.hpp"

#include <filesystem>

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
