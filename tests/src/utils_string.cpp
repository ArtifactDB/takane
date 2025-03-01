#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "takane/utils_string.hpp"

#include "utils.h"

struct Hdf5StringFormatTest : public::testing::Test {
    static std::string testpath() {
        return "TEST_stringformat.h5";
    }

    template<typename ... Args_>
    static void expect_error(const std::string& msg, Args_&& ... args) {
        EXPECT_ANY_THROW({
            try {
                takane::internal_string::validate_string_format(std::forward<Args_>(args)...);
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
        takane::internal_string::validate_string_format(dhandle, 10, "none", {}, 10000);
        expect_error("unsupported format", dhandle, 10, "foobar", std::optional<std::string>{}, 10000);
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
        expect_error("date-formatted string", dhandle, 5, "date", std::optional<std::string>{}, 10000);
        takane::internal_string::validate_string_format(dhandle, 5, "date", "", 10000);
    }

    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        handle.unlink("foobar");
        hdf5_utils::spawn_string_data(handle, "foobar", 10, { "2023-01-05", "1999-12-05", "2002-05-23", "2010-08-18", "1987-06-15" });
    }
    {
        H5::H5File handle(path, H5F_ACC_RDONLY);
        auto dhandle = handle.openDataSet("foobar");
        takane::internal_string::validate_string_format(dhandle, 5, "date", {}, 10000);
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
        expect_error("date-formatted string", dhandle, 5, "date", "foobar", 10000);
        takane::internal_string::validate_string_format(dhandle, 5, "date", "aarontllun", 10000);
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
        expect_error("date/time-formatted string", dhandle, 5, "date-time", std::optional<std::string>{}, 10000);
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
        takane::internal_string::validate_string_format(dhandle, 5, "date-time", {}, 10000);
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
        expect_error("date/time-formatted string", dhandle, 5, "date-time", "foobar", 10000);
        takane::internal_string::validate_string_format(dhandle, 5, "date-time", "aarontllun", 10000);
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
                takane::internal_string::validate_names(std::forward<Args_>(args)...);
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
        takane::internal_string::validate_names(handle, "names", 5, 1000);
        expect_error("same length", handle, "names", 100, 1000);
    }

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        handle.createDataSet("names", H5::PredType::NATIVE_INT, H5S_SCALAR);
    }
    {
        H5::H5File handle(path, H5F_ACC_RDONLY);
        expect_error("represented by a UTF-8 encoded string", handle, "names", 5, 1000);
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
