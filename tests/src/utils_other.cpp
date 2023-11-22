#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "data_frame.h"
#include "simple_list.h"
#include "takane/utils_other.hpp"

#include "utils.h"

struct ValidateMetadataTest : public::testing::Test {
    static std::filesystem::path testdir() {
        return "TEST_validate";
    }

    template<typename ... Args_>
    static void expect_error_mcols(const std::string& msg, Args_&& ... args) {
        EXPECT_ANY_THROW({
            try {
                takane::internal_other::validate_mcols(std::forward<Args_>(args)...);
            } catch (std::exception& e) {
                EXPECT_THAT(e.what(), ::testing::HasSubstr(msg));
                throw;
            }
        });
    }

    template<typename ... Args_>
    static void expect_error_metadata(const std::string& msg, Args_&& ... args) {
        EXPECT_ANY_THROW({
            try {
                takane::internal_other::validate_metadata(std::forward<Args_>(args)...);
            } catch (std::exception& e) {
                EXPECT_THAT(e.what(), ::testing::HasSubstr(msg));
                throw;
            }
        });
    }
};

TEST_F(ValidateMetadataTest, Mcols) {
    auto path = testdir();
    std::filesystem::create_directory(path);
    auto subpath = path / "mcols";
    initialize_directory(subpath, "data_frame");
    data_frame::mock(subpath, 10, true, {});

    takane::internal_other::validate_mcols(path, "mcols", 10, takane::Options());
    expect_error_mcols("unexpected number of rows", path, "mcols", 20, takane::Options());

    initialize_directory(subpath, "simple_list");
    expect_error_mcols("'DATA_FRAME'", path, "mcols", 10, takane::Options());
}

TEST_F(ValidateMetadataTest, Metadata) {
    auto path = testdir();
    std::filesystem::create_directory(path);
    auto subpath = path / "metadata";
    initialize_directory(subpath, "simple_list");
    simple_list::mock(subpath);

    takane::internal_other::validate_metadata(path, "metadata", takane::Options());

    initialize_directory(subpath, "data_frame");
    expect_error_metadata("'SIMPLE_LIST'", path, "metadata", takane::Options());
}
