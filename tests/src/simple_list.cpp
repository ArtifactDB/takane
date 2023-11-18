#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "takane/simple_list.hpp"
#include "takane/validate.hpp"
#include "utils.h"

#include "byteme/byteme.hpp"

#include <string>
#include <filesystem>
#include <fstream>

struct SimpleListTest : public::testing::Test, public Hdf5Utils {
    static std::filesystem::path testdir() {
        return "TEST_simple_list";
    }

    static void initialize() {
        auto dir = testdir();
        if (std::filesystem::exists(dir)) {
            std::filesystem::remove_all(dir);
        }
        std::filesystem::create_directory(dir);
    }

    static void dump_json(const std::string& buffer) {
        auto path = testdir();
        path.append("list_contents.json.gz");
        byteme::GzipFileWriter writer(path.c_str());
        writer.write(reinterpret_cast<const unsigned char*>(buffer.data()), buffer.size());
    }

    template<typename ... Args_>
    static void expect_error(const std::string& msg, Args_&& ... args) {
        EXPECT_ANY_THROW({
            try {
                takane::simple_list::validate(testdir(), std::forward<Args_>(args)...);
            } catch (std::exception& e) {
                EXPECT_THAT(e.what(), ::testing::HasSubstr(msg));
                throw;
            }
        });
    }
};

TEST_F(SimpleListTest, Json) {
    {
        initialize();
    }
    expect_error("could not determine format");

    // Success!
    {
        dump_json("{ \"type\": \"list\", \"values\": [] }");
    }
    takane::simple_list::validate(testdir());

    // Throwing in some externals.
    auto dir = testdir();
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
        attach_attribute(ghandle, "version", "1.0");
        attach_attribute(ghandle, "type", "integer");
        spawn_data(ghandle, "values", 100, H5::PredType::NATIVE_INT32);

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
    takane::simple_list::validate(testdir());
}

TEST_F(SimpleListTest, Hdf5) {
    // Success!
    {
        initialize();
        auto dir = testdir();
        dir.append("list_contents.h5");

        H5::H5File handle(dir, H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup("simple_list");
        H5::StrType stype(0, H5T_VARIABLE);
        auto ahandle = ghandle.createAttribute("uzuki_object", stype, H5S_SCALAR);
        ahandle.write(stype, std::string("list"));
        ghandle.createGroup("data");
    }
    takane::simple_list::validate(testdir());

    // Throwing in some externals.
    auto dir2 = testdir();
    dir2.append("other_contents");
    dir2.append("0");
    {
        std::filesystem::create_directories(dir2);
        auto opath = dir2;
        opath.append("contents.h5");

        H5::H5File handle(opath, H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup("atomic_vector");
        attach_attribute(ghandle, "version", "1.0");
        attach_attribute(ghandle, "type", "integer");
        spawn_data(ghandle, "values", 100, H5::PredType::NATIVE_INT32);

        auto objpath = dir2;
        objpath.append("OBJECT");
        std::ofstream output(objpath);
        output << "atomic_vector";
    }
    expect_error("fewer instances");

    // Success again!
    {
        auto dir = testdir();
        dir.append("list_contents.h5");
        H5::H5File handle(dir, H5F_ACC_TRUNC);
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
    takane::simple_list::validate(testdir());
}
