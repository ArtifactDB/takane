#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "takane/utils_factor.hpp"

#include "utils.h"

struct Hdf5FactorTest : public::testing::Test {
    static std::string testpath() {
        return "TEST_factorutils.h5";
    }

    template<typename ... Args_>
    static void expect_error_levels(const std::string& msg, Args_&& ... args) {
        EXPECT_ANY_THROW({
            try {
                takane::internal_factor::validate_factor_levels(std::forward<Args_>(args)...);
            } catch (std::exception& e) {
                EXPECT_THAT(e.what(), ::testing::HasSubstr(msg));
                throw;
            }
        });
    }

    template<typename ... Args_>
    static void expect_error_codes(const std::string& msg, Args_&& ... args) {
        EXPECT_ANY_THROW({
            try {
                takane::internal_factor::validate_factor_codes(std::forward<Args_>(args)...);
            } catch (std::exception& e) {
                EXPECT_THAT(e.what(), ::testing::HasSubstr(msg));
                throw;
            }
        });
    }
};

TEST_F(Hdf5FactorTest, Levels) {
    auto path = testpath();

    size_t nlevels = 0;
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        hdf5_utils::spawn_data(handle, "fab", 10, H5::PredType::NATIVE_INT32);
        hdf5_utils::spawn_data(handle, "foobar", 10, H5::StrType(0, 10000));

        std::vector<std::string> levels { "A", "BB", "CCC", "DDDD", "EEEEE" };
        nlevels = levels.size();
        hdf5_utils::spawn_string_data(handle, "blah", H5T_VARIABLE, levels);
    }
    {
        H5::H5File handle(path, H5F_ACC_RDONLY);
        expect_error_levels("expected a string", handle, "fab", 10000);
        expect_error_levels("duplicated factor level", handle, "foobar", 10000);
        EXPECT_EQ(takane::internal_factor::validate_factor_levels(handle, "blah", 10000), nlevels);
    }
}

TEST_F(Hdf5FactorTest, Codes) {
    auto path = testpath();

    size_t ncodes = 20;
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        hdf5_utils::spawn_data(handle, "fab", 10, H5::PredType::NATIVE_FLOAT);
        hdf5_utils::spawn_data(handle, "blah", ncodes, H5::PredType::NATIVE_INT32);
    }
    {
        H5::H5File handle(path, H5F_ACC_RDONLY);
        expect_error_codes("32-bit signed integer", handle, "fab", 10, 10000);
        expect_error_codes("less than the number of levels", handle, "blah", 0, 10000);
        EXPECT_EQ(takane::internal_factor::validate_factor_codes(handle, "blah", 10, 10000), ncodes);
    }

    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        auto dhandle = handle.openDataSet("blah");
        std::vector<int> stuff(ncodes);
        for (size_t i = 0; i < ncodes; ++i) {
            stuff[i] = static_cast<int>(i) % 5 - 1;
        }
        dhandle.write(stuff.data(), H5::PredType::NATIVE_INT);
    }
    {
        H5::H5File handle(path, H5F_ACC_RDONLY);
        expect_error_codes("non-negative", handle, "blah", 4, 10000);
    }

    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        auto dhandle = handle.openDataSet("blah");
        auto ahandle = dhandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_INT32, H5S_SCALAR);
        int val = -1;
        ahandle.write(H5::PredType::NATIVE_INT, &val);
    }
    {
        H5::H5File handle(path, H5F_ACC_RDONLY);
        EXPECT_EQ(takane::internal_factor::validate_factor_codes(handle, "blah", 4, 10000), ncodes);
        expect_error_codes("number of levels", handle, "blah", 3, 10000);
    }
}
