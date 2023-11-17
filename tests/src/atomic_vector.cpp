#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "takane/atomic_vector.hpp"
#include "utils.h"

#include <string>
#include <filesystem>

struct AtomicVectorTest : public::testing::Test, public Hdf5Utils {
    static std::filesystem::path testdir() {
        return "TEST_atomic_vector";
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
                takane::atomic_vector::validate(testdir(), std::forward<Args_>(args)...);
            } catch (std::exception& e) {
                EXPECT_THAT(e.what(), ::testing::HasSubstr(msg));
                throw;
            }
        });
    }
};

TEST_F(AtomicVectorTest, Basic) {
    {
        auto handle = initialize();
    }
    expect_error("expected an 'atomic_vector' group");

    {
        auto handle = reopen();
        auto ghandle = handle.createGroup("atomic_vector");
        attach_attribute(ghandle, "version", "2.0");
    }
    expect_error("unsupported version string");

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup("atomic_vector");
        ghandle.removeAttr("version");
        attach_attribute(ghandle, "version", "1.0");
        ghandle.createDataSet("values", H5::PredType::NATIVE_INT, H5S_SCALAR);
    }
    expect_error("1-dimensional dataset");

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup("atomic_vector");
        ghandle.unlink("values");
        spawn_data(ghandle, "values", 100, H5::PredType::NATIVE_INT32);
        attach_attribute(ghandle, "type", "foobar");
    }
    expect_error("unsupported type");

    // Success at last.
    {
        auto handle = reopen();
        auto ghandle = handle.openGroup("atomic_vector");
        ghandle.removeAttr("type");
        attach_attribute(ghandle, "type", "integer");
    }
    takane::atomic_vector::validate(testdir());
}

TEST_F(AtomicVectorTest, TypeChecks) {
    // Integer.
    {
        {
            auto handle = initialize();
            auto ghandle = handle.createGroup("atomic_vector");
            attach_attribute(ghandle, "version", "1.0");
            attach_attribute(ghandle, "type", "integer");
            spawn_data(ghandle, "values", 100, H5::PredType::NATIVE_FLOAT);
        }
        expect_error("32-bit signed integer");

        {
            auto handle = reopen();
            auto ghandle = handle.openGroup("atomic_vector");
            ghandle.unlink("values");
            spawn_data(ghandle, "values", 100, H5::PredType::NATIVE_INT32);
        }
        takane::atomic_vector::validate(testdir());
    }

    // Boolean.
    {
        {
            auto handle = initialize();
            auto ghandle = handle.createGroup("atomic_vector");
            attach_attribute(ghandle, "version", "1.0");
            attach_attribute(ghandle, "type", "boolean");
            spawn_data(ghandle, "values", 100, H5::PredType::NATIVE_FLOAT);
        }
        expect_error("32-bit signed integer");

        {
            auto handle = reopen();
            auto ghandle = handle.openGroup("atomic_vector");
            ghandle.unlink("values");
            spawn_data(ghandle, "values", 100, H5::PredType::NATIVE_INT32);
        }
        takane::atomic_vector::validate(testdir());
    }

    // Number.
    {
        {
            auto handle = initialize();
            auto ghandle = handle.createGroup("atomic_vector");
            attach_attribute(ghandle, "version", "1.0");
            attach_attribute(ghandle, "type", "number");
            spawn_data(ghandle, "values", 100, H5::PredType::NATIVE_INT64);
        }
        expect_error("64-bit float");

        {
            auto handle = reopen();
            auto ghandle = handle.openGroup("atomic_vector");
            ghandle.unlink("values");
            spawn_data(ghandle, "values", 100, H5::PredType::NATIVE_DOUBLE);
        }
        takane::atomic_vector::validate(testdir());
    }

    // String.
    {
        {
            auto handle = initialize();
            auto ghandle = handle.createGroup("atomic_vector");
            attach_attribute(ghandle, "version", "1.0");
            attach_attribute(ghandle, "type", "string");
            spawn_data(ghandle, "values", 100, H5::PredType::NATIVE_INT);
        }
        expect_error("string datatype");

        {
            auto handle = reopen();
            auto ghandle = handle.openGroup("atomic_vector");
            ghandle.unlink("values");
            spawn_data(ghandle, "values", 5, H5::StrType(0, 10));
        }
        takane::atomic_vector::validate(testdir());

        {
            auto handle = reopen();
            auto ghandle = handle.openGroup("atomic_vector");
            attach_attribute(ghandle, "format", "date");
        }
        expect_error("date-formatted string");
    }
}

TEST_F(AtomicVectorTest, MissingChecks) {
    {
        auto handle = initialize();
        auto ghandle = handle.createGroup("atomic_vector");
        attach_attribute(ghandle, "version", "1.0");
        attach_attribute(ghandle, "type", "integer");
        spawn_data(ghandle, "values", 100, H5::PredType::NATIVE_INT32);
        auto dhandle = ghandle.openDataSet("values");
        auto attr = dhandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_FLOAT, H5S_SCALAR);
    }
    expect_error("missing-value-placeholder");

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup("atomic_vector");
        auto dhandle = ghandle.openDataSet("values");
        dhandle.removeAttr("missing-value-placeholder");
        auto attr = dhandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_INT32, H5S_SCALAR);
        int val = -1;
        attr.write(H5::PredType::NATIVE_INT, &val);
    }
    takane::atomic_vector::validate(testdir());
}

TEST_F(AtomicVectorTest, NameChecks) {
    {
        auto handle = initialize();
        auto ghandle = handle.createGroup("atomic_vector");
        attach_attribute(ghandle, "version", "1.0");
        attach_attribute(ghandle, "type", "integer");
        spawn_data(ghandle, "values", 100, H5::PredType::NATIVE_INT32);
        spawn_data(ghandle, "names", 100, H5::PredType::NATIVE_INT32);
    }
    expect_error("string datatype");

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup("atomic_vector");
        ghandle.unlink("names");
        spawn_data(ghandle, "names", 50, H5::StrType(0, 10));
    }
    expect_error("same length");

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup("atomic_vector");
        ghandle.unlink("names");
        spawn_data(ghandle, "names", 100, H5::StrType(0, 10));
    }
    takane::atomic_vector::validate(testdir());
}
