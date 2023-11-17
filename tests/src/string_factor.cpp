#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "takane/string_factor.hpp"
#include "takane/validate.hpp"
#include "utils.h"

#include <string>
#include <filesystem>
#include <fstream>

struct StringFactorTest : public::testing::Test, public Hdf5Utils {
    static std::filesystem::path testdir() {
        return "TEST_string_factor";
    }

    static H5::H5File initialize() {
        auto dir = testdir();
        if (std::filesystem::exists(dir)) {
            std::filesystem::remove_all(dir);
        }
        std::filesystem::create_directory(dir);

        auto path = dir;
        path.append("contents.h5");
        return H5::H5File(std::string(path), H5F_ACC_TRUNC);
    }

    static H5::H5File reopen() {
        auto path = testdir();
        path.append("contents.h5");
        return H5::H5File(path, H5F_ACC_RDWR);
    }

    template<typename ... Args_>
    static void expect_error(const std::string& msg, Args_&& ... args) {
        EXPECT_ANY_THROW({
            try {
                takane::string_factor::validate(testdir(), std::forward<Args_>(args)...);
            } catch (std::exception& e) {
                EXPECT_THAT(e.what(), ::testing::HasSubstr(msg));
                throw;
            }
        });
    }
};

TEST_F(StringFactorTest, Basic) {
    {
        auto handle = initialize();
    }
    expect_error("expected a 'string_factor' group");

    {
        auto handle = reopen();
        handle.createDataSet("string_factor", H5::PredType::NATIVE_INT, H5S_SCALAR);
    }
    expect_error("expected a 'string_factor' group");

    {
        auto handle = reopen();
        handle.unlink("string_factor");
        auto ghandle = handle.createGroup("string_factor");
        attach_attribute(ghandle, "version", "2.0");
    }
    expect_error("unsupported version string");

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup("string_factor");
        ghandle.removeAttr("version");
        attach_attribute(ghandle, "version", "1.0");
    }
    expect_error("'levels'");

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup("string_factor");
        spawn_string_data(ghandle, "levels", 3, { "A", "B", "C", "D", "E" });
    }
    expect_error("'codes'");

    // Success at last.
    {
        auto handle = reopen();
        auto ghandle = handle.openGroup("string_factor");
        spawn_data(ghandle, "codes", 100, H5::PredType::NATIVE_INT32);
    }
    takane::string_factor::validate(testdir());
}

TEST_F(StringFactorTest, CodeChecks) {
    {
        auto handle = initialize();
        auto ghandle = handle.createGroup("string_factor");
        attach_attribute(ghandle, "version", "1.0");

        std::vector<int> codes { 0, -1, 2, 1, 3, -1, 2 };
        auto dhandle = spawn_data(ghandle, "codes", codes.size(), H5::PredType::NATIVE_INT32);
        dhandle.write(codes.data(), H5::PredType::NATIVE_INT);
        spawn_string_data(ghandle, "levels", 3, { "A", "B", "C" });
    }
    expect_error("non-negative");

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup("string_factor");
        auto dhandle = ghandle.openDataSet("codes");
        auto ahandle = dhandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_INT32, H5S_SCALAR);
        int val = -1;
        ahandle.write(H5::PredType::NATIVE_INT, &val);
    }
    expect_error("number of levels");

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup("string_factor");
        ghandle.unlink("levels");
        spawn_string_data(ghandle, "levels", 3, { "A", "B", "C", "D" });
    }
    takane::string_factor::validate(testdir());
}

TEST_F(StringFactorTest, OrderedChecks) {
    {
        auto handle = initialize();
        auto ghandle = handle.createGroup("string_factor");
        attach_attribute(ghandle, "version", "1.0");

        std::vector<int> codes { 0, 2, 1, 1, 2 };
        auto dhandle = spawn_data(ghandle, "codes", codes.size(), H5::PredType::NATIVE_INT32);
        dhandle.write(codes.data(), H5::PredType::NATIVE_INT);
        spawn_string_data(ghandle, "levels", 3, { "A", "B", "C" });

        attach_attribute(ghandle, "ordered", "TRUE");
    }
    expect_error("32-bit signed integer");

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup("string_factor");
        ghandle.removeAttr("ordered");
        auto ahandle = ghandle.createAttribute("ordered", H5::PredType::NATIVE_INT8, H5S_SCALAR);
        int val = 1;
        ahandle.write(H5::PredType::NATIVE_INT, &val);
    }
    takane::string_factor::validate(testdir());
}

TEST_F(StringFactorTest, NameChecks) {
    std::vector<int> codes { 0, 1, 2, 1, 0, 1, 2 };
    {
        auto handle = initialize();
        auto ghandle = handle.createGroup("string_factor");
        attach_attribute(ghandle, "version", "1.0");

        auto dhandle = spawn_data(ghandle, "codes", codes.size(), H5::PredType::NATIVE_INT32);
        dhandle.write(codes.data(), H5::PredType::NATIVE_INT);
        spawn_string_data(ghandle, "levels", 3, { "A", "B", "C" });

        spawn_data(ghandle, "names", codes.size(), H5::PredType::NATIVE_INT);
    }
    expect_error("string datatype");

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup("string_factor");
        ghandle.unlink("names");
        spawn_data(ghandle, "names", 50, H5::StrType(0, 10));
    }
    expect_error("same length");

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup("string_factor");
        ghandle.unlink("names");
        spawn_data(ghandle, "names", codes.size(), H5::StrType(0, 10));
    }
    takane::string_factor::validate(testdir());
}

TEST_F(StringFactorTest, Dispatch) {
    std::vector<int> codes { 0, 1, 2, 1, 0, 1, 2 };
    {
        auto handle = initialize();
        auto ghandle = handle.createGroup("string_factor");
        attach_attribute(ghandle, "version", "1.0");

        auto dhandle = spawn_data(ghandle, "codes", codes.size(), H5::PredType::NATIVE_INT32);
        dhandle.write(codes.data(), H5::PredType::NATIVE_INT);
        spawn_string_data(ghandle, "levels", 3, { "A", "B", "C" });

        auto objpath = testdir();
        objpath.append("OBJECT");
        std::ofstream output(objpath);
        output << "string_factor";
    }
    takane::validate(testdir());
}