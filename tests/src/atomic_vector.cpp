#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "utils.h"
#include "atomic_vector.h"

#include <string>
#include <filesystem>
#include <fstream>

struct AtomicVectorTest : public::testing::Test {
    AtomicVectorTest() {
        dir = "TEST_atomic_vector";
        name = "atomic_vector";
    }

    std::filesystem::path dir;
    std::string name;

    H5::H5File initialize() {
        initialize_directory_simple(dir, name, "1.0");
        return H5::H5File(dir / "contents.h5", H5F_ACC_TRUNC);
    }

    H5::H5File reopen() {
        return H5::H5File(dir / "contents.h5", H5F_ACC_RDWR);
    }

    void expect_error(const std::string& msg) {
        EXPECT_ANY_THROW({
            try {
                test_validate(dir);
            } catch (std::exception& e) {
                EXPECT_THAT(e.what(), ::testing::HasSubstr(msg));
                throw;
            }
        });
    }
};

TEST_F(AtomicVectorTest, Basic) {
    initialize_directory_simple(dir, name, "2.0");
    expect_error("unsupported version string");

    {
        initialize();
        auto handle = reopen();
        auto ghandle = handle.createGroup("atomic_vector");
        ghandle.createDataSet("values", H5::PredType::NATIVE_INT, H5S_SCALAR);
    }
    expect_error("1-dimensional dataset");

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup("atomic_vector");
        ghandle.unlink("values");
        hdf5_utils::spawn_data(ghandle, "values", 100, H5::PredType::NATIVE_INT32);
        hdf5_utils::attach_attribute(ghandle, "type", "foobar");
    }
    expect_error("unsupported type");

    // Success at last.
    {
        auto handle = reopen();
        auto ghandle = handle.openGroup("atomic_vector");
        ghandle.removeAttr("type");
        hdf5_utils::attach_attribute(ghandle, "type", "integer");
    }
    test_validate(dir);
    EXPECT_EQ(test_height(dir), 100);
}

TEST_F(AtomicVectorTest, Types) {
    // Integer.
    {
        mock(dir, 100, atomic_vector::Type::INTEGER);
        test_validate(dir);

        {
            auto handle = reopen();
            auto ghandle = handle.openGroup(name);
            ghandle.unlink("values");
            hdf5_utils::spawn_data(ghandle, "values", 100, H5::PredType::NATIVE_FLOAT);
        }
        expect_error("32-bit signed integer");
    }

    // Boolean.
    {
        mock(dir, 100, atomic_vector::Type::BOOLEAN);
        test_validate(dir);

        {
            auto handle = reopen();
            auto ghandle = handle.openGroup(name);
            ghandle.unlink("values");
            hdf5_utils::spawn_data(ghandle, "values", 100, H5::PredType::NATIVE_FLOAT);
        }
        expect_error("32-bit signed integer");
    }

    // Number.
    {
        mock(dir, 100, atomic_vector::Type::NUMBER);
        test_validate(dir);

        {
            auto handle = reopen();
            auto ghandle = handle.openGroup(name);
            ghandle.unlink("values");
            hdf5_utils::spawn_data(ghandle, "values", 100, H5::PredType::NATIVE_INT64);
        }
        expect_error("64-bit float");
    }

    // String.
    {
        mock(dir, 100, atomic_vector::Type::STRING);
        test_validate(dir);

        {
            auto handle = reopen();
            auto ghandle = handle.openGroup(name);
            ghandle.unlink("values");
            hdf5_utils::spawn_data(ghandle, "values", 100, H5::PredType::NATIVE_INT);
        }
        expect_error("represented by a UTF-8 encoded string");

        {
            mock(dir, 13, atomic_vector::Type::STRING);
            auto handle = reopen();
            auto ghandle = handle.openGroup(name);
            hsize_t dim = 10;
            H5::DataSpace dspace(1, &dim);
            ghandle.createAttribute("format", H5::PredType::NATIVE_INT, dspace);
        }
        expect_error("scalar");

        {
            auto handle = reopen();
            auto ghandle = handle.openGroup("atomic_vector");
            ghandle.removeAttr("format");
            ghandle.createAttribute("format", H5::PredType::NATIVE_INT, H5S_SCALAR);
        }
        expect_error("represented by a UTF-8 encoded string");

        {
            auto handle = reopen();
            auto ghandle = handle.openGroup("atomic_vector");
            ghandle.removeAttr("format");
            hdf5_utils::attach_attribute(ghandle, "format", "date");
        }
        expect_error("date-formatted string");
    }
}

TEST_F(AtomicVectorTest, Missingness) {
    mock(dir, 100, atomic_vector::Type::INTEGER);

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        auto dhandle = ghandle.openDataSet("values");
        auto attr = dhandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_FLOAT, H5S_SCALAR);
    }
    expect_error("missing-value-placeholder");

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        auto dhandle = ghandle.openDataSet("values");
        dhandle.removeAttr("missing-value-placeholder");
        auto attr = dhandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_INT32, H5S_SCALAR);
        int val = -1;
        attr.write(H5::PredType::NATIVE_INT, &val);
    }
    test_validate(dir);
}

TEST_F(AtomicVectorTest, NameChecks) {
    mock(dir, 100, atomic_vector::Type::INTEGER);

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup("atomic_vector");
        hdf5_utils::spawn_data(ghandle, "names", 100, H5::PredType::NATIVE_INT32);
    }
    expect_error("represented by a UTF-8 encoded string");

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup("atomic_vector");
        ghandle.unlink("names");
        hdf5_utils::spawn_data(ghandle, "names", 50, H5::StrType(0, 10));
    }
    expect_error("same length");

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup("atomic_vector");
        ghandle.unlink("names");
        hdf5_utils::spawn_data(ghandle, "names", 100, H5::StrType(0, 10));
    }
    test_validate(dir);
}
