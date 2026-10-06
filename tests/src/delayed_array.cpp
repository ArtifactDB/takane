#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "takane/delayed_array.hpp"
#include "mock_delayed_array.h"
#include "utils.h"

#include <string>
#include <vector>
#include <filesystem>
#include <stdexcept>

TEST(DelayedArray, Okay) {
    auto dir = define_test_path("delayed_array");

    {
        mock_delayed_array(dir, DenseArrayType::INTEGER, { 10, 20 });
    }
    {
        test_validate(dir);
        EXPECT_EQ(test_height(dir), 10);
        std::vector<std::size_t> expected_dims { 10, 20 };
        EXPECT_EQ(test_dimensions(dir), expected_dims);
    }

    // No external references at all.
    {
        initialize_directory_simple(dir, "delayed_array", "1.0");

        H5::H5File handle(dir / "array.h5", H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup("delayed_array");
        add_hdf5_attribute(ghandle, "delayed_type", "array");
        add_hdf5_attribute(ghandle, "delayed_array", "constant array");
        add_hdf5_attribute(ghandle, "delayed_version", "1.1");

        std::vector<hsize_t> dimensions{ 15, 3, 14 };
        auto dhandle = add_hdf5_dataset(ghandle, "dimensions", H5::PredType::NATIVE_UINT32, dimensions.size());
        dhandle.write(dimensions.data(), H5::PredType::NATIVE_HSIZE);

        auto thandle = ghandle.createDataSet("value", H5::PredType::NATIVE_INT8, H5S_SCALAR);
        add_hdf5_attribute(thandle, "type", "BOOLEAN");
    }
    {
        test_validate(dir);
        EXPECT_EQ(test_height(dir), 15);
        std::vector<std::size_t> expected_dims { 15, 3, 14 };
        EXPECT_EQ(test_dimensions(dir), expected_dims);
    }

    // Multiple external references.
    {
        initialize_directory_simple(dir, "delayed_array", "1.0");

        H5::H5File handle(dir / "array.h5", H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup("delayed_array");
        add_hdf5_attribute(ghandle, "delayed_type", "operation");
        add_hdf5_attribute(ghandle, "delayed_operation", "combine");
        add_hdf5_attribute(ghandle, "delayed_version", "1.1");

        auto ahandle = ghandle.createDataSet("along", H5::PredType::NATIVE_UINT32, H5S_SCALAR);
        int along = 1;
        ahandle.write(&along, H5::PredType::NATIVE_INT);

        std::filesystem::create_directory(dir / "seeds");
        auto shandle = ghandle.createGroup("seeds");
        int len = 3;
        auto attr = shandle.createAttribute("length", H5::PredType::NATIVE_UINT32, H5S_SCALAR);
        attr.write(H5::PredType::NATIVE_INT, &len);

        for (int i = 0; i < len; ++i) {
            auto nm = std::to_string(i);
            auto xhandle = shandle.createGroup(nm);
            add_hdf5_attribute(xhandle, "delayed_type", "array");
            add_hdf5_attribute(xhandle, "delayed_array", "custom takane seed array");

            H5::StrType stype(0, H5T_VARIABLE);
            auto thandle = xhandle.createDataSet("type", stype, H5S_SCALAR);
            thandle.write(std::string("INTEGER"), stype);

            std::vector<hsize_t> dims(2);
            dims[0] = 10;
            dims[1] = i * 20;
            auto dhandle = add_hdf5_dataset(xhandle, "dimensions", H5::PredType::NATIVE_UINT32, dims.size());
            dhandle.write(dims.data(), H5::PredType::NATIVE_HSIZE);

            auto ihandle = xhandle.createDataSet("index", H5::PredType::NATIVE_UINT32, H5S_SCALAR);
            ihandle.write(&i, H5::PredType::NATIVE_INT);
            mock_dense_array(dir / "seeds" / nm, DenseArrayType::INTEGER, std::move(dims));
        }
    }
    {
        test_validate(dir);
        EXPECT_EQ(test_height(dir), 10);
        std::vector<std::size_t> expected_dims { 10, 60 };
        EXPECT_EQ(test_dimensions(dir), expected_dims);
    }
}

TEST(DelayedArray, VersionError) {
    auto dir = define_test_path("delayed_array");

    // Check the version of the object.
    {
        initialize_directory_simple(dir, "delayed_array", "2.0");
    }
    expect_validation_error(dir, "unsupported version");

    // Fails if the version is too old.
    {
        auto ghandle = mock_delayed_array(dir, DenseArrayType::INTEGER, { 10, 20 });
        ghandle.removeAttr("delayed_version");
    }
    expect_validation_error(dir, "no less than 1.1");
}

TEST(DelayedArray, ChihayaError) {
    auto dir = define_test_path("delayed_array");

    {
        initialize_directory_simple(dir, "delayed_array", "1.0");
        H5::H5File handle(dir / "array.h5", H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup("delayed_array");
        add_hdf5_attribute(ghandle, "delayed_type", "array");
        add_hdf5_attribute(ghandle, "delayed_array", "unknown array");
        add_hdf5_attribute(ghandle, "delayed_version", "1.1");
    }
    expect_validation_error(dir, "failed to validate 'delayed_array' in 'array.h5'");
}

TEST(DelayedArray, IndexError) {
    auto dir = define_test_path("delayed_array");

    {
        auto ghandle = mock_delayed_array(dir, DenseArrayType::INTEGER, { 10, 20 });
        ghandle.unlink("index");
        add_hdf5_dataset(ghandle, "index", H5::PredType::NATIVE_UINT8, 20);
    }
    expect_validation_error(dir, "scalar");

    {
        auto ghandle = mock_delayed_array(dir, DenseArrayType::INTEGER, { 10, 20 });
        ghandle.unlink("index");
        ghandle.createDataSet("index", H5::PredType::NATIVE_INT, H5S_SCALAR);
    }
    expect_validation_error(dir, "64-bit unsigned integer");

    {
        mock_delayed_array(dir, DenseArrayType::INTEGER, { 10, 20 });
        mock_dense_array(dir / "seeds" / "3", DenseArrayType::INTEGER, { 10, 20 });
    }
    expect_validation_error(dir, "number of objects in 'seeds' is not consistent");

    // Trying with a single valid index that doesn't start at zero.
    {
        auto ghandle = mock_delayed_array(dir, DenseArrayType::INTEGER, { 10, 20 });
        ghandle.unlink("index");
        auto dhandle = ghandle.createDataSet("index", H5::PredType::NATIVE_UINT64, H5S_SCALAR);
        int val = 3;
        dhandle.write(&val, H5::PredType::NATIVE_INT);

        std::filesystem::remove_all(dir / "seeds" / "0");
        mock_dense_array(dir / "seeds" / "3", DenseArrayType::INTEGER, { 10, 20 });
        EXPECT_EQ(takane::count_directory_entries(dir / "seeds"), 1);
    }
    expect_validation_error(dir, "number of objects in 'seeds' is not consistent");
}

TEST(DelayedArray, DimensionsError) {
    auto dir = define_test_path("delayed_array");

    {
        mock_delayed_array(dir, DenseArrayType::INTEGER, { 10, 20 });
        std::filesystem::remove_all(dir / "seeds" / "0");
        mock_dense_array(dir / "seeds" / "0", DenseArrayType::INTEGER, { 10, 20, 5 });
    }
    expect_validation_error(dir, "dimensionality is not consistent");

    {
        mock_delayed_array(dir, DenseArrayType::INTEGER, { 10, 20 });
        std::filesystem::remove_all(dir / "seeds" / "0");
        mock_dense_array(dir / "seeds" / "0", DenseArrayType::INTEGER, { 10, 5 });
    }
    expect_validation_error(dir, "dimension extents");

    {
        mock_delayed_array(dir, DenseArrayType::INTEGER, { 20, 5 });
        std::filesystem::remove_all(dir / "seeds" / "0");
        mock_dense_array(dir / "seeds" / "0", DenseArrayType::INTEGER, { 10, 5 });
    }
    expect_validation_error(dir, "dimension extents");
}

TEST(DelayedArray, OverrideError) {
    auto dir = define_test_path("delayed_array");

    {
        mock_delayed_array(dir, DenseArrayType::INTEGER, { 1, 2, 3 });
    }

    // Check that we respect any custom overrides for the takane seed.
    takane::Options opt;
    opt.delayed_array_options.array_validate_registry["custom takane seed array"] = [&](const H5::Group&, const ritsuko::Version&, const chihaya::Options&) -> chihaya::ArrayDetails {
        throw std::runtime_error("WHOOOOO");
    };
    expect_validation_error(dir, "WHOOOOO", opt);
}

