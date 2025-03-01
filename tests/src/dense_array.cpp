#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "takane/dense_array.hpp"
#include "utils.h"
#include "dense_array.h"

#include <string>
#include <vector>
#include <filesystem>
#include <stdexcept>

struct DenseArrayTest : public ::testing::Test {
    DenseArrayTest() {
        dir = "TEST_dense_array";
        name = "dense_array";
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

TEST_F(DenseArrayTest, Basics) {
    initialize_directory_simple(dir, name, "2.0");
    expect_error("unsupported version");

    // Success!
    {
        dense_array::mock(dir, dense_array::Type::INTEGER, { 10, 20 });
    }
    test_validate(dir);
    EXPECT_EQ(test_height(dir), 10);

    std::vector<size_t> expected_dims { 10, 20 };
    EXPECT_EQ(test_dimensions(dir), expected_dims);
}

TEST_F(DenseArrayTest, TypeChecks) {
    {
        dense_array::mock(dir, dense_array::Type::INTEGER, { 10, 20 });
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        ghandle.removeAttr("type");
        hdf5_utils::attach_attribute(ghandle, "type", "string");
    }
    expect_error("represented by a UTF-8 encoded string");

    {
        dense_array::mock(dir, dense_array::Type::NUMBER, { 10, 20 });
    }
    test_validate(dir);

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        ghandle.removeAttr("type");
        hdf5_utils::attach_attribute(ghandle, "type", "integer");
    }
    expect_error("32-bit signed integer");

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        ghandle.removeAttr("type");
        hdf5_utils::attach_attribute(ghandle, "type", "boolean");
    }
    expect_error("32-bit signed integer");

    {
        dense_array::mock(dir, dense_array::Type::STRING, { 10, 20 });
    }
    test_validate(dir);

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        ghandle.removeAttr("type");
        hdf5_utils::attach_attribute(ghandle, "type", "number");
    }
    expect_error("64-bit float");

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        ghandle.removeAttr("type");
        hdf5_utils::attach_attribute(ghandle, "type", "YAYYA");
    }
    expect_error("unknown array type");
}

TEST_F(DenseArrayTest, NullStrings) {
    // Check for NULL pointers.
    {
        initialize_directory_simple(dir, name, "1.0");
        H5::H5File handle(dir / "array.h5", H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup(name);

        std::vector<hsize_t> dims { 10, 20 };
        H5::DataSpace dspace(dims.size(), dims.data());
        ghandle.createDataSet("data", H5::StrType(0, H5T_VARIABLE), dspace);
        hdf5_utils::attach_attribute(ghandle, "type", "string");
    }
    expect_error("NULL pointer");

    // Doesn't throw if it's empty.
    {
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        ghandle.unlink("data");
        ghandle.removeAttr("type");

        std::vector<hsize_t> dims { 10, 0 };
        H5::DataSpace dspace(dims.size(), dims.data());
        ghandle.createDataSet("data", H5::StrType(0, H5T_VARIABLE), dspace);
        hdf5_utils::attach_attribute(ghandle, "type", "string");
    }
    test_validate(dir);

    // Works if it's compressed.
    {
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        ghandle.unlink("data");
        ghandle.removeAttr("type");

        std::vector<hsize_t> chunks { 10, 20 };
        H5::DSetCreatPropList cplist;
        cplist.setChunk(2, chunks.data());
        cplist.setDeflate(6);

        std::vector<hsize_t> dims { 33, 45 };
        H5::DataSpace dspace(dims.size(), dims.data());
        ghandle.createDataSet("data", H5::StrType(0, H5T_VARIABLE), dspace, cplist);
        hdf5_utils::attach_attribute(ghandle, "type", "string");
    }
    expect_error("NULL pointer");
}

TEST_F(DenseArrayTest, MissingPlaceholder) {
    {
        dense_array::mock(dir, dense_array::Type::INTEGER, { 10, 20 });
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        auto dhandle = ghandle.openDataSet("data");
        hdf5_utils::attach_attribute(dhandle, "missing-value-placeholder", "NA");
    }
    expect_error("same type as its dataset");

    {
        dense_array::mock(dir, dense_array::Type::STRING, { 10, 20 });
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        auto dhandle = ghandle.openDataSet("data");
        hdf5_utils::attach_attribute(dhandle, "missing-value-placeholder", "NA");
    }
    test_validate(dir);
}

TEST_F(DenseArrayTest, Names) {
    {
        dense_array::mock(dir, dense_array::Type::INTEGER, { 10, 20 });
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        auto nhandle = ghandle.createGroup("names");
        hdf5_utils::spawn_data(nhandle, "0", 20, H5::StrType(0, 5));
    }
    expect_error("same length as the extent");

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        auto nhandle = ghandle.openGroup("names");
        nhandle.unlink("0");
        hdf5_utils::spawn_data(nhandle, "1", 20, H5::StrType(0, 5));
    }
    test_validate(dir);
}

TEST_F(DenseArrayTest, Transposed) {
    {
        dense_array::mock(dir, dense_array::Type::INTEGER, { 10, 20 });
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        hdf5_utils::attach_attribute(ghandle, "transposed", "123123");
    }
    expect_error("32-bit signed integer");

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        ghandle.removeAttr("transposed");
        hsize_t foo = 10;
        H5::DataSpace dspace(1, &foo);
        ghandle.createAttribute("transposed", H5::PredType::NATIVE_INT32, dspace);
    }
    expect_error("scalar");

    {
        dense_array::mock(dir, dense_array::Type::INTEGER, { 10, 20 });
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        ghandle.createAttribute("transposed", H5::PredType::NATIVE_INT32, H5S_SCALAR);
    }
    test_validate(dir);
    EXPECT_EQ(test_height(dir), 10);

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        auto ahandle = ghandle.openAttribute("transposed");
        int val = 1;
        ahandle.write(H5::PredType::NATIVE_INT, &val);
    }
    test_validate(dir);
    EXPECT_EQ(test_height(dir), 20);

    std::vector<size_t> expected_dims { 20, 10 };
    EXPECT_EQ(test_dimensions(dir), expected_dims);
}

struct DenseArrayStringCheckTest : public ::testing::TestWithParam<int> {};

TEST_P(DenseArrayStringCheckTest, NullCheck) {
    std::filesystem::path dir = "TEST_dense_array";
    std::string name = "dense_array";

    // To check correct iteration, our strategy is to add a NULL pointer at every corner,
    // turn the buffer size down, and verify that everything is touched.
    std::vector<hsize_t> dims { 10, 20, 5 };
    const char* dummy = "Aaron";
    takane::Options options;
    options.hdf5_buffer_size = GetParam();

    H5::DataSpace dspace(dims.size(), dims.data());
    for (size_t i = 0; i < 2; ++i) {
        for (size_t j = 0; j < 2; ++j) {
            for (size_t k = 0; k < 2; ++k) {
                std::vector<const char*> ptrs(1000, dummy);

                // Note that we go in reverse order of dimensions as HDF5 stores the fastest-changing dimension last.
                size_t multiplier = 1;
                size_t offset = k * (dims[2] - 1) * multiplier;
                multiplier *= dims[2];
                offset += j * (dims[1] - 1) * multiplier;
                multiplier *= dims[1];
                offset += i * (dims[0] - 1) * multiplier;

                ptrs[offset] = NULL;

                {
                    initialize_directory_simple(dir, name, "1.0");
                    H5::H5File handle(dir / "array.h5", H5F_ACC_TRUNC);
                    auto ghandle = handle.createGroup(name);
                    hdf5_utils::attach_attribute(ghandle, "type", "string");
                    auto dhandle = ghandle.createDataSet("data", H5::StrType(0, H5T_VARIABLE), dspace);
                    dhandle.write(ptrs.data(), H5::StrType(0, H5T_VARIABLE));
                }

                auto meta = takane::read_object_metadata(dir);
                EXPECT_ANY_THROW({
                    try {
                        takane::dense_array::validate(dir, meta, options);
                    } catch (std::exception& e) {
                        EXPECT_THAT(e.what(), ::testing::HasSubstr("NULL pointer"));
                        throw;
                    }
                });
            }
        }
    }
}

INSTANTIATE_TEST_SUITE_P(
    DenseArray,
    DenseArrayStringCheckTest,
    ::testing::Values(2, 5, 10, 20, 100) // buffer size.
);

TEST_F(DenseArrayTest, Vls) {
    std::string heap = "abcdefghijklmno";
    std::vector<size_t> expected_dimensions { 3, 4 };

    {
        initialize_directory_simple(dir, "dense_array", "1.1");
        H5::H5File handle(dir / "array.h5", H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup("dense_array");
        hdf5_utils::attach_attribute(ghandle, "type", "vls");

        const unsigned char* hptr = reinterpret_cast<const unsigned char*>(heap.c_str());
        hsize_t hlen = heap.size();
        H5::DataSpace hspace(1, &hlen);
        auto hhandle = ghandle.createDataSet("heap", H5::PredType::NATIVE_UINT8, hspace);
        hhandle.write(hptr, H5::PredType::NATIVE_UCHAR);

        std::vector<ritsuko::hdf5::vls::Pointer<uint64_t, uint64_t> > pointers(expected_dimensions[0] * expected_dimensions[1]);
        for (size_t i = 0; i < pointers.size(); ++i) {
            pointers[i].offset = i; 
            pointers[i].length = 1;
        }
        std::vector<hsize_t> pdims(expected_dimensions.begin(), expected_dimensions.end());
        H5::DataSpace pspace(2, pdims.data());
        auto ptype = ritsuko::hdf5::vls::define_pointer_datatype<uint64_t, uint64_t>();
        auto phandle = ghandle.createDataSet("pointers", ptype, pspace);
        phandle.write(pointers.data(), ptype);
    }

    test_validate(dir);
    EXPECT_EQ(test_dimensions(dir), expected_dimensions);

    // Adding a missing value placeholder.
    {
        {
            H5::H5File handle(dir / "array.h5", H5F_ACC_RDWR);
            auto ghandle = handle.openGroup("dense_array");
            auto dhandle = ghandle.openDataSet("pointers");
            dhandle.createAttribute("missing-value-placeholder", H5::StrType(0, 10), H5S_SCALAR);
        }
        test_validate(dir);

        // Adding the wrong missing value placeholder.
        {
            H5::H5File handle(dir / "array.h5", H5F_ACC_RDWR);
            auto ghandle = handle.openGroup("dense_array");
            auto dhandle = ghandle.openDataSet("pointers");
            dhandle.removeAttr("missing-value-placeholder");
            dhandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_INT, H5S_SCALAR);
        }
        expect_error("string datatype");

        // Removing for the next checks.
        {
            H5::H5File handle(dir / "array.h5", H5F_ACC_RDWR);
            auto ghandle = handle.openGroup("dense_array");
            auto dhandle = ghandle.openDataSet("pointers");
            dhandle.removeAttr("missing-value-placeholder");
        }
    }

    // Checking that this only works in the latest version.
    {
        auto opath = dir/"OBJECT";
        auto parsed = millijson::parse_file(opath.c_str());
        auto& entries = reinterpret_cast<millijson::Object*>(parsed.get())->values;
        auto& av_entries = reinterpret_cast<millijson::Object*>(entries["dense_array"].get())->values;
        reinterpret_cast<millijson::String*>(av_entries["version"].get())->value = "1.0";
        json_utils::dump(parsed.get(), opath);

        expect_error("unsupported type");

        reinterpret_cast<millijson::String*>(av_entries["version"].get())->value = "1.1";
        json_utils::dump(parsed.get(), opath);
    }

    // Shortening the heap to check that we perform bounds checks on the pointers.
    {
        {
            H5::H5File handle(dir / "array.h5", H5F_ACC_RDWR);
            auto ghandle = handle.openGroup("dense_array");
            ghandle.unlink("heap");
            hsize_t zero = 0;
            H5::DataSpace hspace(1, &zero);
            ghandle.createDataSet("heap", H5::PredType::NATIVE_UINT8, hspace);
        }
        expect_error("out of range");
    }

    // Checking that we check for 64-bit unsigned integer types. 
    {
        {
            H5::H5File handle(dir / "array.h5", H5F_ACC_RDWR);
            auto ghandle = handle.openGroup("dense_array");
            ghandle.unlink("pointers");

            std::vector<ritsuko::hdf5::vls::Pointer<int, int> > pointers(expected_dimensions[0] * expected_dimensions[1]);
            for (auto& p : pointers) {
                p.offset = 0;
                p.length = 0;
            }
            std::vector<hsize_t> pdims(expected_dimensions.begin(), expected_dimensions.end());
            H5::DataSpace pspace(2, pdims.data());
            auto ptype = ritsuko::hdf5::vls::define_pointer_datatype<int, int>();
            auto phandle = ghandle.createDataSet("pointers", ptype, pspace);
            phandle.write(pointers.data(), ptype);
        }
        expect_error("64-bit unsigned integer");
    }
}
