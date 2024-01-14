#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "takane/delayed_array.hpp"
#include "delayed_array.h"
#include "compressed_sparse_matrix.h"
#include "data_frame.h"
#include "utils.h"

#include <string>
#include <vector>
#include <filesystem>
#include <stdexcept>

struct DelayedArrayTest : public ::testing::Test {
    DelayedArrayTest() {
        dir = "TEST_delayed_array";
        name = "delayed_array";
    }

    std::filesystem::path dir;
    std::string name;

    H5::H5File reopen() {
        return H5::H5File(dir / "array.h5", H5F_ACC_RDWR);
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

TEST_F(DelayedArrayTest, Basics) {
    initialize_directory_simple(dir, name, "2.0");
    expect_error("unsupported version");

    // Success!
    {
        delayed_array::mock(dir, dense_array::Type::INTEGER, { 10, 20 });
    }
    test_validate(dir);
    EXPECT_EQ(test_height(dir), 10);

    std::vector<size_t> expected_dims { 10, 20 };
    EXPECT_EQ(test_dimensions(dir), expected_dims);

    // Support the other types...
    delayed_array::mock(dir, dense_array::Type::BOOLEAN, { 10, 20 });
    test_validate(dir);

    delayed_array::mock(dir, dense_array::Type::NUMBER, { 10, 20 });
    test_validate(dir);

    delayed_array::mock(dir, dense_array::Type::STRING, { 10, 20 });
    test_validate(dir);
}

TEST_F(DelayedArrayTest, IndexChecks) {
    {
        delayed_array::mock(dir, dense_array::Type::INTEGER, { 10, 20 });
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        ghandle.unlink("index");
        ghandle.createDataSet("index", H5::PredType::NATIVE_INT, H5S_SCALAR);
    }
    expect_error("64-bit unsigned integer");

    auto seed_path = dir / "seeds";
    {
        delayed_array::mock(dir, dense_array::Type::INTEGER, { 10, 20 });
        dense_array::mock(seed_path / "3", dense_array::Type::INTEGER, { 10, 20 });
    }
    expect_error("number of objects in 'seeds' is not consistent");

    {
        std::filesystem::remove_all(seed_path / "0");
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        ghandle.unlink("index");
        auto dhandle = ghandle.createDataSet("index", H5::PredType::NATIVE_UINT64, H5S_SCALAR);
        int val = 3;
        dhandle.write(&val, H5::PredType::NATIVE_INT);
    }
    EXPECT_EQ(takane::internal_other::count_directory_entries(seed_path), 1);
    expect_error("number of objects in 'seeds' is not consistent");

    // Forcibly creating a more interesting delayed array.
    {
        initialize_directory_simple(dir, "delayed_array", "1.0");
        H5::H5File handle(dir / "array.h5", H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup("delayed_array");
        hdf5_utils::attach_attribute(ghandle, "delayed_type", "operation");
        hdf5_utils::attach_attribute(ghandle, "delayed_operation", "combine");
        hdf5_utils::attach_attribute(ghandle, "delayed_version", "1.1");

        auto ahandle = ghandle.createDataSet("along", H5::PredType::NATIVE_UINT32, H5S_SCALAR);
        int along = 1;
        ahandle.write(&along, H5::PredType::NATIVE_INT);

        auto seed_path = dir / "seeds";
        std::filesystem::create_directory(seed_path);

        auto shandle = ghandle.createGroup("seeds");
        int len = 3;
        auto attr = shandle.createAttribute("length", H5::PredType::NATIVE_UINT32, H5S_SCALAR);
        attr.write(H5::PredType::NATIVE_INT, &len);

        for (int i = 0; i < len; ++i) {
            auto nm = std::to_string(i);
            auto xhandle = shandle.createGroup(nm);
            hdf5_utils::attach_attribute(xhandle, "delayed_type", "array");
            hdf5_utils::attach_attribute(xhandle, "delayed_array", "custom takane seed array");

            H5::StrType stype(0, H5T_VARIABLE);
            auto thandle = xhandle.createDataSet("type", stype, H5S_SCALAR);
            thandle.write(std::string("INTEGER"), stype);

            std::vector<hsize_t> dims(2);
            dims[0] = 10;
            dims[1] = i * 20;
            auto dhandle = hdf5_utils::spawn_data(xhandle, "dimensions", dims.size(), H5::PredType::NATIVE_UINT32);
            dhandle.write(dims.data(), H5::PredType::NATIVE_HSIZE);

            auto ihandle = xhandle.createDataSet("index", H5::PredType::NATIVE_UINT32, H5S_SCALAR);
            ihandle.write(&i, H5::PredType::NATIVE_INT);
            dense_array::mock(seed_path / nm, dense_array::Type::INTEGER, std::move(dims));
        }
    }
    test_validate(dir);
}

TEST_F(DelayedArrayTest, DimensionalityChecks) {
    auto seed_path = dir / "seeds";
    {
        delayed_array::mock(dir, dense_array::Type::INTEGER, { 10, 20 });
        dense_array::mock(seed_path / "0", dense_array::Type::INTEGER, { 10, 20, 5 });
    }
    expect_error("dimensionality");

    {
        delayed_array::mock(dir, dense_array::Type::INTEGER, { 10, 20 });
        dense_array::mock(seed_path / "0", dense_array::Type::INTEGER, { 10, 5 });
    }
    expect_error("dimension extents");
}
