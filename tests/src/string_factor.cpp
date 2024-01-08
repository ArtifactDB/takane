#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "utils.h"

#include <string>
#include <filesystem>
#include <fstream>

struct StringFactorTest : public::testing::Test {
    StringFactorTest() {
        dir = "TEST_string_factor";
        name = "string_factor";
    }

    std::filesystem::path dir;
    std::string name;

    H5::H5File initialize() {
        initialize_directory_simple(dir, "string_factor", "1.0");
        return H5::H5File(dir / "contents.h5", H5F_ACC_TRUNC);
    }

    H5::H5File reopen() {
        auto path = dir / "contents.h5";
        return H5::H5File(path, H5F_ACC_RDWR);
    }

    template<typename ... Args_>
    void expect_error(const std::string& msg, Args_&& ... args) {
        EXPECT_ANY_THROW({
            try {
                test_validate(dir, std::forward<Args_>(args)...);
            } catch (std::exception& e) {
                EXPECT_THAT(e.what(), ::testing::HasSubstr(msg));
                throw;
            }
        });
    }
};

TEST_F(StringFactorTest, Basic) {
    initialize_directory_simple(dir, name, "2.0");
    expect_error("unsupported version string");

    {
        auto handle = initialize();
        auto ghandle = handle.createGroup(name);
    }
    expect_error("'levels'");

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        hdf5_utils::spawn_string_data(ghandle, "levels", 3, { "A", "B", "C", "D", "E" });
    }
    expect_error("'codes'");

    // Success at last.
    {
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        hdf5_utils::spawn_data(ghandle, "codes", 100, H5::PredType::NATIVE_UINT32);
    }
    test_validate(dir);
    EXPECT_EQ(test_height(dir), 100);
}

TEST_F(StringFactorTest, Codes) {
    {
        auto handle = initialize();
        auto ghandle = handle.createGroup(name);

        std::vector<int> codes { 0, 3, 2, 1, 3, 0, 2 };
        auto dhandle = hdf5_utils::spawn_data(ghandle, "codes", codes.size(), H5::PredType::NATIVE_UINT32);
        dhandle.write(codes.data(), H5::PredType::NATIVE_INT);
        hdf5_utils::spawn_string_data(ghandle, "levels", 3, { "A", "B", "C" });
    }
    expect_error("number of levels");

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        auto dhandle = ghandle.openDataSet("codes");
        auto ahandle = dhandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_UINT32, H5S_SCALAR);
        int val = 3;
        ahandle.write(H5::PredType::NATIVE_INT, &val);
    }
    test_validate(dir);
}

TEST_F(StringFactorTest, Ordered) {
    {
        auto handle = initialize();
        auto ghandle = handle.createGroup(name);

        std::vector<int> codes { 0, 2, 1, 1, 2 };
        auto dhandle = hdf5_utils::spawn_data(ghandle, "codes", codes.size(), H5::PredType::NATIVE_UINT32);
        dhandle.write(codes.data(), H5::PredType::NATIVE_INT);
        hdf5_utils::spawn_string_data(ghandle, "levels", 3, { "A", "B", "C" });

        hdf5_utils::attach_attribute(ghandle, "ordered", "TRUE");
    }
    expect_error("32-bit signed integer");

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        ghandle.removeAttr("ordered");

        hsize_t dim = 10;
        H5::DataSpace dspace(1, &dim);
        ghandle.createAttribute("ordered", H5::PredType::NATIVE_INT, dspace);
    }
    expect_error("scalar");

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        ghandle.removeAttr("ordered");
        auto ahandle = ghandle.createAttribute("ordered", H5::PredType::NATIVE_INT8, H5S_SCALAR);
        int val = 1;
        ahandle.write(H5::PredType::NATIVE_INT, &val);
    }
    test_validate(dir);
}

TEST_F(StringFactorTest, Names) {
    std::vector<int> codes { 0, 1, 2, 1, 0, 1, 2 };
    {
        auto handle = initialize();
        auto ghandle = handle.createGroup(name);

        auto dhandle = hdf5_utils::spawn_data(ghandle, "codes", codes.size(), H5::PredType::NATIVE_UINT32);
        dhandle.write(codes.data(), H5::PredType::NATIVE_INT);
        hdf5_utils::spawn_string_data(ghandle, "levels", 3, { "A", "B", "C" });

        hdf5_utils::spawn_data(ghandle, "names", codes.size(), H5::PredType::NATIVE_INT);
    }
    expect_error("represented by a UTF-8 encoded string");

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        ghandle.unlink("names");
        hdf5_utils::spawn_data(ghandle, "names", 50, H5::StrType(0, 10));
    }
    expect_error("same length");

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        ghandle.unlink("names");
        hdf5_utils::spawn_data(ghandle, "names", codes.size(), H5::StrType(0, 10));
    }
    test_validate(dir);
}
