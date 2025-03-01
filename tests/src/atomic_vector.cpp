#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "utils.h"
#include "atomic_vector.h"

#include "ritsuko/hdf5/vls/vls.hpp"

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
    expect_error("expected an attribute");

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup("atomic_vector");
        hdf5_utils::attach_attribute(ghandle, "type", "integer");
    }
    expect_error("1-dimensional dataset");

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup("atomic_vector");
        ghandle.unlink("values");
        hdf5_utils::spawn_data(ghandle, "values", 100, H5::PredType::NATIVE_INT32);
        ghandle.removeAttr("type");
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

TEST_F(AtomicVectorTest, Vls) {
    std::string heap = "abcdefghijklmno";

    {
        initialize_directory_simple(dir, "atomic_vector", "1.1");
        H5::H5File handle(dir / "contents.h5", H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup("atomic_vector");
        hdf5_utils::attach_attribute(ghandle, "type", "vls");

        const unsigned char* hptr = reinterpret_cast<const unsigned char*>(heap.c_str());
        hsize_t hlen = heap.size();
        H5::DataSpace hspace(1, &hlen);
        auto hhandle = ghandle.createDataSet("heap", H5::PredType::NATIVE_UINT8, hspace);
        hhandle.write(hptr, H5::PredType::NATIVE_UCHAR);

        std::vector<ritsuko::hdf5::vls::Pointer<uint64_t, uint64_t> > pointers(3);
        pointers[0].offset = 0; pointers[0].length = 5;
        pointers[1].length = 5; pointers[1].length = 7;
        pointers[1].length = 12; pointers[1].length = 3;
        hsize_t plen = pointers.size();
        H5::DataSpace pspace(1, &plen);
        auto ptype = ritsuko::hdf5::vls::define_pointer_datatype<uint64_t, uint64_t>();
        auto phandle = ghandle.createDataSet("pointers", ptype, pspace);
        phandle.write(pointers.data(), ptype);
    }

    test_validate(dir);
    EXPECT_EQ(test_height(dir), 3);

    // Adding a missing value placeholder.
    {
        {
            H5::H5File handle(dir / "contents.h5", H5F_ACC_RDWR);
            auto ghandle = handle.openGroup("atomic_vector");
            auto dhandle = ghandle.openDataSet("pointers");
            dhandle.createAttribute("missing-value-placeholder", H5::StrType(0, 10), H5S_SCALAR);
        }
        test_validate(dir);

        // Adding the wrong missing value placeholder.
        {
            H5::H5File handle(dir / "contents.h5", H5F_ACC_RDWR);
            auto ghandle = handle.openGroup("atomic_vector");
            auto dhandle = ghandle.openDataSet("pointers");
            dhandle.removeAttr("missing-value-placeholder");
            dhandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_INT, H5S_SCALAR);
        }
        expect_error("string datatype");

        // Removing for the next checks.
        {
            H5::H5File handle(dir / "contents.h5", H5F_ACC_RDWR);
            auto ghandle = handle.openGroup("atomic_vector");
            auto dhandle = ghandle.openDataSet("pointers");
            dhandle.removeAttr("missing-value-placeholder");
        }
    }

    // Checking that this only works in the latest version.
    {
        auto opath = dir/"OBJECT";
        auto parsed = millijson::parse_file(opath.c_str());
        auto& entries = reinterpret_cast<millijson::Object*>(parsed.get())->values;
        auto& av_entries = reinterpret_cast<millijson::Object*>(entries["atomic_vector"].get())->values;
        reinterpret_cast<millijson::String*>(av_entries["version"].get())->value = "1.0";
        json_utils::dump(parsed.get(), opath);

        expect_error("unsupported type");

        reinterpret_cast<millijson::String*>(av_entries["version"].get())->value = "1.1";
        json_utils::dump(parsed.get(), opath);
    }

    // Shortening the heap to check that we perform bounds checks on the pointers.
    {
        {
            H5::H5File handle(dir / "contents.h5", H5F_ACC_RDWR);
            auto ghandle = handle.openGroup("atomic_vector");
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
            H5::H5File handle(dir / "contents.h5", H5F_ACC_RDWR);
            auto ghandle = handle.openGroup("atomic_vector");
            ghandle.unlink("pointers");

            std::vector<ritsuko::hdf5::vls::Pointer<int, int> > pointers(3);
            for (auto& p : pointers) {
                p.offset = 0;
                p.length = 0;
            }
            hsize_t plen = pointers.size();
            H5::DataSpace pspace(1, &plen);
            auto ptype = ritsuko::hdf5::vls::define_pointer_datatype<int, int>();
            auto phandle = ghandle.createDataSet("pointers", ptype, pspace);
            phandle.write(pointers.data(), ptype);
        }
        expect_error("64-bit unsigned integer");
    }
}
