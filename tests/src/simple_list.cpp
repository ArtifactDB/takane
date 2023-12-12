#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "takane/takane.hpp"
#include "utils.h"
#include "simple_list.h"

#include <string>
#include <filesystem>
#include <fstream>

struct SimpleListTest : public::testing::Test {
    SimpleListTest() {
        dir = "TEST_simple_list";
        name = "simple_list";
    }

    std::filesystem::path dir;
    std::string name;

    void dump_json(const std::string& buffer) {
        simple_list::dump_compressed_json(dir, buffer);
    }

    template<typename ... Args_>
    void expect_error(const std::string& msg, Args_&& ... args) {
        EXPECT_ANY_THROW({
            try {
                takane::validate(dir, std::forward<Args_>(args)...);
            } catch (std::exception& e) {
                EXPECT_THAT(e.what(), ::testing::HasSubstr(msg));
                throw;
            }
        });
    }
};

TEST_F(SimpleListTest, Basics) {
    simple_list::initialize_with_metadata(dir, "2.0", "whee");
    expect_error("unsupported version");

    simple_list::initialize_with_metadata(dir, "1.0", "whee");
    expect_error("unsupported format");
}

TEST_F(SimpleListTest, Json) {
    // Success!
    {
        dump_json("{ \"type\": \"list\", \"values\": [] }");
    }
    takane::validate(dir);
    EXPECT_EQ(takane::height(dir), 0);

    // Throwing in some externals.
    dir.append("other_contents");
    {
        std::ofstream x(dir);
    }
    expect_error("expected 'other_contents' to be a directory");

    auto dir2 = dir;
    dir2.append("0");
    {
        std::filesystem::remove(dir);
        std::filesystem::create_directory(dir);
        std::ofstream x(dir2);
    }
    expect_error("failed to validate external list object at 'other_contents/0'");

    {
        std::filesystem::remove(dir2);
        std::filesystem::create_directory(dir2);
        auto opath = dir2;
        opath.append("contents.h5");

        H5::H5File handle(opath, H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup("atomic_vector");
        hdf5_utils::attach_attribute(ghandle, "version", "1.0");
        hdf5_utils::attach_attribute(ghandle, "type", "integer");
        hdf5_utils::spawn_data(ghandle, "values", 100, H5::PredType::NATIVE_INT32);

        auto objpath = dir2;
        objpath.append("OBJECT");
        std::ofstream output(objpath);
        output << "atomic_vector";
    }
    expect_error("fewer instances");

    // Success again!
    {
        dump_json("{ \"type\": \"list\", \"values\": [ { \"type\": \"external\", \"index\": 0 } ] }");
    }
    takane::validate(dir);
    EXPECT_EQ(takane::height(dir), 1);
}

TEST_F(SimpleListTest, Hdf5) {
    // Success!
    {
        simple_list::initialize_with_metadata(dir, "1.0", "hdf5");
        dir.append("list_contents.h5");

        H5::H5File handle(dir, H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup("simple_list");
        H5::StrType stype(0, H5T_VARIABLE);
        auto ahandle = ghandle.createAttribute("uzuki_object", stype, H5S_SCALAR);
        ahandle.write(stype, std::string("list"));
        ghandle.createGroup("data");
    }
    takane::validate(dir);
    EXPECT_EQ(takane::height(dir), 0);

    // Still works with an implicit default format of HDF5.
    dump_object_metadata_simple(dir, "simple_list", "1.0");
    takane::validate(dir);

    // Throwing in some externals.
    auto dir2 = dir;
    dir2.append("other_contents");
    dir2.append("0");
    {
        std::filesystem::create_directories(dir2);
        auto opath = dir2;
        opath.append("contents.h5");

        H5::H5File handle(opath, H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup("atomic_vector");
        hdf5_utils::attach_attribute(ghandle, "version", "1.0");
        hdf5_utils::attach_attribute(ghandle, "type", "integer");
        hdf5_utils::spawn_data(ghandle, "values", 100, H5::PredType::NATIVE_INT32);

        auto objpath = dir2;
        objpath.append("OBJECT");
        std::ofstream output(objpath);
        output << "atomic_vector";
    }
    expect_error("fewer instances");

    // Actually referencing those externals, so we get success again!
    {
        H5::H5File handle(dir / "list_contents.h5", H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup("simple_list");

        H5::StrType stype(0, H5T_VARIABLE);
        auto ahandle = ghandle.createAttribute("uzuki_object", stype, H5S_SCALAR);
        ahandle.write(stype, std::string("list"));

        auto dhandle = ghandle.createGroup("data");
        auto zhandle = dhandle.createGroup("0");
        {
            auto xhandle = zhandle.createAttribute("uzuki_object", stype, H5S_SCALAR);
            xhandle.write(stype, std::string("external"));
        }

        auto xhandle = zhandle.createDataSet("index", H5::PredType::NATIVE_INT32, H5S_SCALAR);
        int val = 0;
        xhandle.write(&val, H5::PredType::NATIVE_INT);
    }
    takane::validate(dir);
    EXPECT_EQ(takane::height(dir), 1);
}
