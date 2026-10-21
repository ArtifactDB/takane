#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "utils.h"

#include <filesystem>

TEST(Validate, Fail) {
    auto dir = define_test_path("_validate");

    {
        initialize_directory_simple(dir, "foobar", "1.0");
    }
    expect_validation_error(dir, "no registered validation function");
}

TEST(Validate, Customized) {
    auto dir = define_test_path("_validate");

    {
        initialize_directory_simple(dir, "foobar", "1.0");
    }

    {
        takane::Options opts;
        opts.custom_validate["foobar"] = [](const std::filesystem::path&, const takane::ObjectMetadata&, const takane::Options&) -> void {};
        test_validate(dir, opts);

        // Throw an error to make sure it actually runs.
        opts.custom_validate["foobar"] = [](const std::filesystem::path&, const takane::ObjectMetadata&, const takane::Options&) -> void { throw std::runtime_error("YAY"); };
        expect_validation_error(dir, "YAY", opts);
    }

    // Test global validators.
    {
        takane::Options opts;
        opts.custom_validate["foobar"] = [](const std::filesystem::path&, const takane::ObjectMetadata&, const takane::Options&) -> void {};
        opts.custom_global_validate = [](const std::filesystem::path&, const takane::ObjectMetadata&, const takane::Options&) -> void {};
        test_validate(dir, opts);

        opts.custom_global_validate = [](const std::filesystem::path&, const takane::ObjectMetadata&, const takane::Options&) -> void { throw std::runtime_error("ARGH"); };
        expect_validation_error(dir, "ARGH", opts);
    }
}
