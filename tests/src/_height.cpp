#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "utils.h"

#include <filesystem>

TEST(Height, Error) {
    auto dir = define_test_path("height");

    {
        initialize_directory_simple(dir, "foobar", "1.0");
    }

    expect_error(
        "no registered 'height' function",
        [&]() -> void {
            test_height(dir);
        }
    );
}

TEST(Height, Custom) {
    auto dir = define_test_path("height");

    {
        initialize_directory_simple(dir, "foobar", "1.0");
    }

    takane::Options opts;
    opts.custom_height["foobar"] = [](const std::filesystem::path&, const takane::ObjectMetadata&, const takane::Options&) -> size_t { return 11; };
    EXPECT_EQ(test_height(dir, opts), 11);
}
