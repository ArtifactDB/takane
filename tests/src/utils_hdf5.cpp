#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "takane/utils_hdf5.hpp"

#include "utils.h"

struct Hdf5StringFormatTest : public::testing::Test {
    static std::string testpath() {
        return "TEST_stringformat.h5";
    }

    template<typename ... Args_>
    static void expect_error(const std::string& msg, Args_&& ... args) {
        EXPECT_ANY_THROW({
            try {
                takane::internal_hdf5::validate_string_format(std::forward<Args_>(args)...);
            } catch (std::exception& e) {
                EXPECT_THAT(e.what(), ::testing::HasSubstr(msg));
                throw;
            }
        });
    }
};

TEST_F(Hdf5StringFormatTest, None) {
    auto path = testpath();

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        hdf5_utils::spawn_data(handle, "foobar", 10, H5::StrType(0, 10));
    }
    {
        H5::H5File handle(path, H5F_ACC_RDONLY);
        auto dhandle = handle.openDataSet("foobar");
        takane::internal_hdf5::validate_string_format(dhandle, 10, "none", false, "", 10000);
        expect_error("unsupported format", dhandle, 10, "foobar", false, "", 10000);
    }
}

TEST_F(Hdf5StringFormatTest, Date) {
    auto path = testpath();

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        hdf5_utils::spawn_data(handle, "foobar", 5, H5::StrType(0, 10)); // must be 10 characters.
    }
    {
        H5::H5File handle(path, H5F_ACC_RDONLY);
        auto dhandle = handle.openDataSet("foobar");
        expect_error("date-formatted string", dhandle, 5, "date", false, "", 10000);
        takane::internal_hdf5::validate_string_format(dhandle, 5, "date", true, "", 10000);
    }

    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        handle.unlink("foobar");
        hdf5_utils::spawn_string_data(handle, "foobar", 10, { "2023-01-05", "1999-12-05", "2002-05-23", "2010-08-18", "1987-06-15" });
    }
    {
        H5::H5File handle(path, H5F_ACC_RDONLY);
        auto dhandle = handle.openDataSet("foobar");
        takane::internal_hdf5::validate_string_format(dhandle, 5, "date", false, "", 10000);
    }

    // Checking for missing placeholder.
    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        auto dhandle = handle.openDataSet("foobar");

        hsize_t len = 1;
        H5::DataSpace memspace(1, &len);
        H5::DataSpace filespace = dhandle.getSpace();
        hsize_t start = 4;
        filespace.selectHyperslab(H5S_SELECT_SET, &len, &start);

        std::string placeholder = "aarontllun"; // must be 10 characters.
        dhandle.write(placeholder.c_str(), dhandle.getStrType(), memspace, filespace);
    }
    {
        H5::H5File handle(path, H5F_ACC_RDONLY);
        auto dhandle = handle.openDataSet("foobar");
        expect_error("date-formatted string", dhandle, 5, "date", true, "foobar", 10000);
        takane::internal_hdf5::validate_string_format(dhandle, 5, "date", true, "aarontllun", 10000);
    }
}

TEST_F(Hdf5StringFormatTest, DateTime) {
    auto path = testpath();

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto dhandle = hdf5_utils::spawn_data(handle, "foobar", 5, H5::StrType(0, H5T_VARIABLE));
        std::vector<std::string> contents { "A", "BB", "CCC", "DDDD", "EEEEEE" };
        auto ptrs = hdf5_utils::pointerize_strings(contents);
        dhandle.write(ptrs.data(), dhandle.getStrType());
    }
    {
        H5::H5File handle(path, H5F_ACC_RDONLY);
        auto dhandle = handle.openDataSet("foobar");
        expect_error("date/time-formatted string", dhandle, 5, "date-time", false, "", 10000);
    }

    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        auto dhandle = handle.openDataSet("foobar");
        std::vector<std::string> contents;
        for (size_t i = 0; i < 5; ++i) {
            contents.push_back("2023-01-1" + std::to_string(i) + "T00:00:00Z");
        }
        auto ptrs = hdf5_utils::pointerize_strings(contents);
        dhandle.write(ptrs.data(), dhandle.getStrType());
    }
    {
        H5::H5File handle(path, H5F_ACC_RDONLY);
        auto dhandle = handle.openDataSet("foobar");
        takane::internal_hdf5::validate_string_format(dhandle, 5, "date-time", false, "", 10000);
    }

    // Checking for missing placeholder.
    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        auto dhandle = handle.openDataSet("foobar");

        hsize_t len = 1;
        H5::DataSpace memspace(1, &len);
        H5::DataSpace filespace = dhandle.getSpace();
        hsize_t start = 4;
        filespace.selectHyperslab(H5S_SELECT_SET, &len, &start);

        std::string placeholder = "aarontllun"; // must be 10 characters.
        auto pptr = placeholder.c_str();
        dhandle.write(&pptr, dhandle.getStrType(), memspace, filespace);
    }
    {
        H5::H5File handle(path, H5F_ACC_RDONLY);
        auto dhandle = handle.openDataSet("foobar");
        expect_error("date/time-formatted string", dhandle, 5, "date-time", true, "foobar", 10000);
        takane::internal_hdf5::validate_string_format(dhandle, 5, "date-time", true, "aarontllun", 10000);
    }
}

struct Hdf5FactorTest : public::testing::Test {
    static std::string testpath() {
        return "TEST_factorutils.h5";
    }

    template<typename ... Args_>
    static void expect_error_levels(const std::string& msg, Args_&& ... args) {
        EXPECT_ANY_THROW({
            try {
                takane::internal_hdf5::validate_factor_levels(std::forward<Args_>(args)...);
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
                takane::internal_hdf5::validate_factor_codes(std::forward<Args_>(args)...);
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
        EXPECT_EQ(takane::internal_hdf5::validate_factor_levels(handle, "blah", 10000), nlevels);
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
        EXPECT_EQ(takane::internal_hdf5::validate_factor_codes(handle, "blah", 10, 10000), ncodes);
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
        EXPECT_EQ(takane::internal_hdf5::validate_factor_codes(handle, "blah", 4, 10000), ncodes);
        expect_error_codes("number of levels", handle, "blah", 3, 10000);
    }
}

struct Hdf5NamesTest : public::testing::Test {
    static std::string testpath() {
        return "TEST_names_utils.h5";
    }

    template<typename ... Args_>
    static void expect_error(const std::string& msg, Args_&& ... args) {
        EXPECT_ANY_THROW({
            try {
                takane::internal_hdf5::validate_names(std::forward<Args_>(args)...);
            } catch (std::exception& e) {
                EXPECT_THAT(e.what(), ::testing::HasSubstr(msg));
                throw;
            }
        });
    }
};

TEST_F(Hdf5NamesTest, Basics) {
    auto path = testpath();

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        hdf5_utils::spawn_data(handle, "names", 5, H5::StrType(0, 10)); // must be 10 characters.
    }
    {
        H5::H5File handle(path, H5F_ACC_RDONLY);
        takane::internal_hdf5::validate_names(handle, "names", 5, 1000);
        expect_error("same length", handle, "names", 100, 1000);
    }

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        handle.createDataSet("names", H5::PredType::NATIVE_INT, H5S_SCALAR);
    }
    {
        H5::H5File handle(path, H5F_ACC_RDONLY);
        expect_error("string datatype class", handle, "names", 5, 1000);
    }

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        hsize_t len = 10;
        H5::DataSpace dspace(1, &len);
        handle.createDataSet("names", H5::StrType(0, H5T_VARIABLE), dspace);
    }
    {
        H5::H5File handle(path, H5F_ACC_RDONLY);
        expect_error("NULL pointer", handle, "names", 10, 1000);
    }
}
