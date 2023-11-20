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
    initialize_directory(path, "data_frame");
    data_frame::mock(path, 10, true, {});
    takane::internal_other::validate_mcols(path, 10, takane::Options());
    expect_error_mcols("unexpected number of rows", path, 20, takane::Options());

    initialize_directory(path, "simple_list");
    expect_error_mcols("'DATA_FRAME'", path, 10, takane::Options());
}

TEST_F(ValidateMetadataTest, Metadata) {
    auto path = testdir();
    initialize_directory(path, "simple_list");
    simple_list::mock(path);
    takane::internal_other::validate_metadata(path, takane::Options());

    initialize_directory(path, "data_frame");
    expect_error_metadata("'SIMPLE_LIST'", path, takane::Options());
}
