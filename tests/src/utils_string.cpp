#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "takane/utils_string.hpp"

#include "utils.h"

TEST(OpenAndLoadStringScalarAttribute, Okay) {
    auto path = define_test_path("utils_string"); 

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup("foo");
        add_hdf5_attribute(ghandle, "bar", "stuff");
    }

    H5::H5File handle(path, H5F_ACC_RDONLY);
    auto ghandle = handle.openGroup("foo");
    EXPECT_EQ(takane::open_and_load_scalar_string_attribute(ghandle, "bar"), "stuff");
}

TEST(OpenAndLoadStringScalarAttribute, Error) {
    auto path = define_test_path("utils_string"); 

    // Check shape.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup("foo");
        constexpr hsize_t one = 1;
        ghandle.createAttribute("bar", H5::StrType(0, H5T_VARIABLE), H5::DataSpace(1, &one));
    }
    {
        H5::H5File handle(path, H5F_ACC_RDONLY);
        auto ghandle = handle.openGroup("foo");
        expect_error("scalar", [&]() -> void {
            takane::open_and_load_scalar_string_attribute(ghandle, "bar");
        });
    }

    // Check type.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup("foo");
        ghandle.createAttribute("bar", H5::PredType::NATIVE_INT32, H5S_SCALAR);
    }
    {
        H5::H5File handle(path, H5F_ACC_RDONLY);
        auto ghandle = handle.openGroup("foo");
        expect_error("UTF-8 encoded string", [&]() -> void {
            takane::open_and_load_scalar_string_attribute(ghandle, "bar");
        });
    }
}

/****************************************/

TEST(OpenAndLoadStringFormat, Okay) {
    auto path = define_test_path("utils_string"); 

    // Returns 'none' when the format attribute isn't present.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        handle.createGroup("foo");
    }
    {
        H5::H5File handle(path, H5F_ACC_RDONLY);
        auto ghandle = handle.openGroup("foo");
        EXPECT_EQ(takane::open_and_load_string_format(ghandle), "none");
    }

    // Otherwise returns the format attribute's contents.
    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        auto ghandle = handle.openGroup("foo");
        add_hdf5_attribute(ghandle, "format", "whee");
    }
    {
        H5::H5File handle(path, H5F_ACC_RDONLY);
        auto ghandle = handle.openGroup("foo");
        EXPECT_EQ(takane::open_and_load_string_format(ghandle), "whee");
    }
}

/****************************************/

template<typename ... Args_>
void expect_validate_string_format_error(const std::string& msg, Args_&& ... args) {
    expect_error(
        msg,
        [&]() -> void {
            takane::validate_string_format(std::forward<Args_>(args)...);
        }
    );
}

TEST(ValidateStringFormat, NoneOkay) {
    auto path = define_test_path("utils_string"); 
    std::optional<std::string> empty_missing;

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        add_hdf5_dataset(handle, "foobar", H5::StrType(0, 10), 10);
    }

    H5::H5File handle(path, H5F_ACC_RDONLY);
    auto dhandle = handle.openDataSet("foobar");
    takane::validate_string_format(dhandle, 10, "none", empty_missing, /* buffer_size = */ 10000);
}

TEST(ValidateStringFormat, NoneError) {
    auto path = define_test_path("utils_string");
    std::optional<std::string> empty_missing;

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        add_hdf5_dataset(handle, "foobar", H5::StrType(0, 10), 10);
    }
    {
        H5::H5File handle(path, H5F_ACC_RDONLY);
        auto dhandle = handle.openDataSet("foobar");
        expect_validate_string_format_error("unsupported format", dhandle, 10, "foobar", empty_missing, /* buffer_size = */ 10000);
    }

    // Check that we actually validate the VLS pointers.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        add_hdf5_dataset(handle, "stuff", H5::StrType(0, H5T_VARIABLE), 10);
    }
    {
        H5::H5File handle(path, H5F_ACC_RDONLY);
        auto dhandle = handle.openDataSet("stuff");
        expect_validate_string_format_error("NULL", dhandle, 10, "none", empty_missing, /* buffer_size = */ 10000);
    }
}

TEST(ValidateStringFormat, DateSimple) {
    auto path = define_test_path("utils_string");
    std::optional<std::string> empty_missing;

    std::vector<std::string> payload{ "2023-01-05", "1999-12-05", "2002-05-23", "2010-08-18", "1987-06-15" };
    auto ptrs = pointerize_strings(payload);

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto dhandle = add_hdf5_dataset(handle, "foobar", H5::StrType(0, H5T_VARIABLE), payload.size());
        dhandle.write(ptrs.data(), H5::StrType(0, H5T_VARIABLE));
    }

    H5::H5File handle(path, H5F_ACC_RDONLY);
    auto dhandle = handle.openDataSet("foobar");

    // We'll use both a large and small buffer size to test that our iteration through the stream is done correctly.
    takane::validate_string_format(dhandle, payload.size(), "date", empty_missing, /* buffer_size = */ 10000);
    takane::validate_string_format(dhandle, payload.size(), "date", empty_missing, /* buffer_size = */ 2);
}

TEST(ValidateStringFormat, DateError) {
    auto path = define_test_path("utils_string");
    std::optional<std::string> empty_missing;

    const char* ref = "2091-11-23";
    const char* dummy = "WHEE";
    std::vector<const char*> ptrs(10, ref);

    for (int i = 0; i < 2; ++i) {
        // Inserting an error at different locations to confirm that the iteration works as expected.
        std::size_t loc;
        if (i == 0) {
            loc = 0;
        } else if ( i== 1) {
            loc = ptrs.size() / 2;
        } else {
            loc = ptrs.size() - 1;
        }
        ptrs[loc] = dummy;

        {
            H5::H5File handle(path, H5F_ACC_TRUNC);
            auto dhandle = add_hdf5_dataset(handle, "foobar", H5::StrType(0, H5T_VARIABLE), ptrs.size());
            dhandle.write(ptrs.data(), H5::StrType(0, H5T_VARIABLE));
        }

        H5::H5File handle(path, H5F_ACC_RDWR);
        auto dhandle = handle.openDataSet("foobar");
        expect_validate_string_format_error("date-formatted string", dhandle, ptrs.size(), "date", empty_missing, /* buffer_size = */ 10000);
        expect_validate_string_format_error("date-formatted string", dhandle, ptrs.size(), "date", empty_missing, /* buffer_size = */ 2);

        ptrs[loc] = ref;
    }
}

TEST(ValidateStringFormat, DateMissing) {
    auto path = define_test_path("utils_string");
    std::optional<std::string> empty_missing;

    const char* ref = "2001-01-09";
    const char* dummy = "WHEE";
    std::vector<const char*> ptrs(5, ref);
    ptrs[0] = dummy;
    ptrs[2] = dummy;
    ptrs[4] = dummy;

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto dhandle = add_hdf5_dataset(handle, "foobar", H5::StrType(0, H5T_VARIABLE), ptrs.size());
        dhandle.write(ptrs.data(), H5::StrType(0, H5T_VARIABLE));
    }

    H5::H5File handle(path, H5F_ACC_RDONLY);
    auto dhandle = handle.openDataSet("foobar");
    expect_validate_string_format_error("date-formatted string", dhandle, ptrs.size(), "date", empty_missing, /* buffer_size = */ 10000);
    expect_validate_string_format_error("date-formatted string", dhandle, ptrs.size(), "date", empty_missing, /* buffer_size = */ 2);

    takane::validate_string_format(dhandle, ptrs.size(), "date", "WHEE", 10000);
}

TEST(ValidateStringFormat, DateTimeSimple) {
    auto path = define_test_path("utils_string");
    std::optional<std::string> empty_missing;

    std::vector<std::string> contents;
    for (size_t i = 0; i < 9; ++i) {
        contents.push_back("2023-01-1" + std::to_string(i) + "T00:00:00Z");
    }

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ptrs = pointerize_strings(contents);
        auto dhandle = add_hdf5_dataset(handle, "foobar", H5::StrType(0, H5T_VARIABLE), ptrs.size());
        dhandle.write(ptrs.data(), H5::StrType(0, H5T_VARIABLE));
    }

    H5::H5File handle(path, H5F_ACC_RDONLY);
    auto dhandle = handle.openDataSet("foobar");
    takane::validate_string_format(dhandle, contents.size(), "date-time", empty_missing, /* buffer_size = */ 10000);
    takane::validate_string_format(dhandle, contents.size(), "date-time", empty_missing, /* buffer_size = */ 2);
}

TEST(ValidateStringFormat, DateTimeError) {
    auto path = define_test_path("utils_string");
    std::optional<std::string> empty_missing;

    const char* ref = "2022-12-21T21:12:22+22:00";
    std::vector<const char*> ptrs(13, ref);

    for (int i = 0; i < 2; ++i) {
        // Inserting an error at different locations to confirm that the iteration works as expected.
        std::size_t loc;
        if (i == 0) {
            loc = 0;
            ptrs[loc] = "2022-12-23";
        } else if (i == 1) {
            loc = ptrs.size() / 2;
            ptrs[loc] = "whee";
        } else {
            loc = ptrs.size() - 1;
            ptrs[loc] = "foobar";
        }

        {
            H5::H5File handle(path, H5F_ACC_TRUNC);
            auto dhandle = add_hdf5_dataset(handle, "foobar", H5::StrType(0, H5T_VARIABLE), ptrs.size());
            dhandle.write(ptrs.data(), H5::StrType(0, H5T_VARIABLE));
        }

        H5::H5File handle(path, H5F_ACC_RDWR);
        auto dhandle = handle.openDataSet("foobar");
        expect_validate_string_format_error("date/time-formatted string", dhandle, ptrs.size(), "date-time", empty_missing, /* buffer_size = */ 10000);
        expect_validate_string_format_error("date/time-formatted string", dhandle, ptrs.size(), "date-time", empty_missing, /* buffer_size = */ 2);

        ptrs[loc] = ref;
    }
}

TEST(ValidateStringFormat, DateTimeMissing) {
    auto path = define_test_path("utils_string");

    const char* ref = "2001-01-09T01:01:01+01:11";
    const char* dummy = "stuff";
    std::vector<const char*> ptrs(8, ref);
    ptrs[1] = dummy;
    ptrs[7] = dummy;

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto dhandle = add_hdf5_dataset(handle, "foobar", H5::StrType(0, H5T_VARIABLE), ptrs.size());
        dhandle.write(ptrs.data(), H5::StrType(0, H5T_VARIABLE));
    }

    std::optional<std::string> empty_missing;
    H5::H5File handle(path, H5F_ACC_RDONLY);
    auto dhandle = handle.openDataSet("foobar");
    expect_validate_string_format_error("date/time-formatted string", dhandle, ptrs.size(), "date-time", empty_missing, /* buffer_size = */ 10000);
    expect_validate_string_format_error("date/time-formatted string", dhandle, ptrs.size(), "date-time", empty_missing, /* buffer_size = */ 2);

    takane::validate_string_format(dhandle, ptrs.size(), "date-time", "stuff", 10000);
}

/****************************************/

template<typename ... Args_>
void expect_validate_names_error(const std::string& msg, Args_&& ... args) {
    expect_error(
        msg,
        [&]() -> void {
            takane::validate_names(std::forward<Args_>(args)...);
        }
    );
}

TEST(ValidateNames, Okay) {
    auto path = define_test_path("utils_string");

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        add_hdf5_dataset(handle, "names", H5::StrType(0, 10), 5);
    }

    H5::H5File handle(path, H5F_ACC_RDONLY);
    takane::validate_names(handle, "names", 5, 1000);
}

TEST(ValidateNames, Error) {
    auto path = define_test_path("utils_string");

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        add_hdf5_dataset(handle, "names", H5::PredType::NATIVE_INT32, 5);
    }
    {
        H5::H5File handle(path, H5F_ACC_RDONLY);
        expect_validate_names_error("UTF-8 encoded string", handle, "names", 5, 1000);
    }

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        handle.createDataSet("names", H5::StrType(0, 10), H5S_SCALAR);
    }
    {
        H5::H5File handle(path, H5F_ACC_RDONLY);
        expect_validate_names_error("1-dimensional", handle, "names", 100, 1000);
    }

    // Check that the length is consistent with the height of the object it's naming.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        add_hdf5_dataset(handle, "names", H5::StrType(0, 10), 5);
    }
    {
        H5::H5File handle(path, H5F_ACC_RDONLY);
        expect_validate_names_error("same length", handle, "names", 100, 1000);
    }

    // Check that we actually validate the strings.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        add_hdf5_dataset(handle, "names", H5::StrType(0, H5T_VARIABLE), 5);
    }
    {
        H5::H5File handle(path, H5F_ACC_RDONLY);
        expect_validate_names_error("NULL", handle, "names", 5, 1000);
    }
}
