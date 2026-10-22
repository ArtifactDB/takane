#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "utils.h"

#include <filesystem>

TEST(Dimensions, Error) {
    auto dir = define_test_path("_height");

    {
        initialize_directory_simple(dir, "foobar", "1.0");
    }
    expect_error(
        "no registered 'dimensions' function",
        [&]() -> void {
            test_dimensions(dir);
        }
    );
}

TEST(Dimensions, Custom) {
    auto dir = define_test_path("_height");

    {
        initialize_directory_simple(dir, "foobar", "1.0");
    }

    std::vector<std::size_t> expected { 11, 20 };
    takane::Options opts;
    opts.custom_dimensions["foobar"] = [&](const std::filesystem::path&, const takane::ObjectMetadata&, const takane::Options&) -> std::vector<std::size_t> {
        return expected;
    };
    EXPECT_EQ(test_dimensions(dir, opts), expected);
}
