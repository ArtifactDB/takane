#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "utils.h"
#include "mock_atomic_vector.h"
#include "mock_simple_list.h"

#include "takane/simple_list.hpp"

#include <string>
#include <filesystem>
#include <fstream>

TEST(ExtractSimpleListFormat, Basic) {
    takane::JsonObjectMap tmp;
    EXPECT_EQ(takane::extract_simple_list_format(tmp), "hdf5");

    tmp["format"] = std::make_shared<millijson::Number>(10);
    expect_error(
        "JSON string",
        [&]() -> void {
            takane::extract_simple_list_format(tmp);
        }
    );

    tmp["format"] = std::make_shared<millijson::String>("foobar");
    EXPECT_EQ(takane::extract_simple_list_format(tmp), "foobar");
}

TEST(ExtractSimpleListLength, Basic) {
    takane::JsonObjectMap tmp;
    EXPECT_FALSE(takane::extract_simple_list_length(tmp).has_value());

    tmp["length"] = std::make_shared<millijson::String>("foobar");
    expect_error(
        "JSON number",
        [&]() -> void {
            takane::extract_simple_list_length(tmp);
        }
    );

    tmp["length"] = std::make_shared<millijson::Number>(10);
    EXPECT_EQ(takane::extract_simple_list_length(tmp), 10);

    tmp["length"] = std::make_shared<millijson::Number>(0.5);
    expect_error(
        "integer",
        [&]() -> void {
            takane::extract_simple_list_length(tmp);
        }
    );

    tmp["length"] = std::make_shared<millijson::Number>(-1);
    expect_error(
        "negative",
        [&]() -> void {
            takane::extract_simple_list_length(tmp);
        }
    );
}

/*****************************************/

TEST(SimpleList, JsonOkay) {
    auto dir = define_test_path("simple_list");
    initialize_simple_list_with_metadata(dir, "1.0", "json.gz");

    {
        dump_compressed_json(dir, "{ \"type\": \"list\", \"values\": [] }");
    }
    test_validate(dir);
    EXPECT_EQ(test_height(dir), 0);

    {
        dump_compressed_json(dir, "{ \"type\": \"list\", \"values\": [ { \"type\": \"integer\", \"values\": 2 }, {\"type\": \"nothing\"} ] }");
    }
    test_validate(dir);
    EXPECT_EQ(test_height(dir), 2);

    // Throwing in some externals.
    {
        dump_compressed_json(dir, "{ \"type\": \"list\", \"values\": [ { \"type\": \"external\", \"index\": 0 } ] }");
        auto odir = dir / "other_contents";
        initialize_directory(odir);
        mock_atomic_vector(odir / "0", 23, AtomicVectorType::INTEGER);
    }
    test_validate(dir);
    EXPECT_EQ(test_height(dir), 1);
}

TEST(SimpleList, JsonLength) {
    auto dir = define_test_path("simple_list");
    initialize_directory(dir);

    // Validates against the length, if supplied.
    {
        std::ofstream output(dir / "OBJECT");
        output << "{ \"type\": \"simple_list\", \"simple_list\": { \"version\": \"1.1\", \"format\": \"json.gz\", \"length\": 3 } }";
        dump_compressed_json(dir, "{ \"type\": \"list\", \"values\": [ {\"type\":\"nothing\"}, {\"type\":\"integer\",\"values\":[2,3]}, {\"type\":\"nothing\"} ] }");
    }
    test_validate(dir);
    EXPECT_EQ(test_height(dir), 3);

    // But also ignores it if no length is supplied.
    {
        std::ofstream output(dir / "OBJECT");
        output << "{ \"type\": \"simple_list\", \"simple_list\": { \"version\": \"1.1\", \"format\": \"json.gz\" } }";
    }
    test_validate(dir);
    EXPECT_EQ(test_height(dir), 3);
}

TEST(SimpleList, JsonError) {
    auto dir = define_test_path("simple_list");

    // Check that we actually run through uzuki2's validator.
    {
        initialize_simple_list_with_metadata(dir, "1.0", "json.gz");
        dump_compressed_json(dir, "{ \"type\": \"integer\", \"values\": [] }");
    }
    expect_validation_error(dir, "top-level");
}

/*****************************************/

TEST(SimpleList, Hdf5Okay) {
    auto dir = define_test_path("simple_list");

    {
        initialize_simple_list_with_metadata(dir, "1.0", "hdf5");
        H5::H5File handle(dir / "list_contents.h5", H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup("simple_list");
        add_hdf5_attribute(ghandle, "uzuki_object", "list");
        ghandle.createGroup("data");
    }
    test_validate(dir);
    EXPECT_EQ(test_height(dir), 0);

    {
        initialize_simple_list_with_metadata(dir, "1.0", "hdf5");
        H5::H5File handle(dir / "list_contents.h5", H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup("simple_list");
        add_hdf5_attribute(ghandle, "uzuki_object", "list");
        auto dhandle = ghandle.createGroup("data");
        {
            auto ghandle = dhandle.createGroup("0");
            add_hdf5_attribute(ghandle, "uzuki_object", "vector");
            add_hdf5_attribute(ghandle, "uzuki_type", "integer");
            add_hdf5_dataset(ghandle, "data", H5::PredType::NATIVE_INT32, 10);
        }
        {
            auto ghandle = dhandle.createGroup("1");
            add_hdf5_attribute(ghandle, "uzuki_object", "nothing");
        }
    }
    test_validate(dir);
    EXPECT_EQ(test_height(dir), 2);

    // Throwing in some externals.
    {
        initialize_simple_list_with_metadata(dir, "1.0", "hdf5");
        H5::H5File handle(dir / "list_contents.h5", H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup("simple_list");
        add_hdf5_attribute(ghandle, "uzuki_object", "list");
        auto dhandle = ghandle.createGroup("data");
        {
            auto ghandle = dhandle.createGroup("0");
            add_hdf5_attribute(ghandle, "uzuki_object", "external");
            auto xhandle = ghandle.createDataSet("index", H5::PredType::NATIVE_INT8, H5S_SCALAR);
            const int val = 0;
            xhandle.write(&val, H5::PredType::NATIVE_INT);
        }
        auto odir = dir / "other_contents";
        initialize_directory(odir);
        mock_atomic_vector(odir / "0", 23, AtomicVectorType::INTEGER);
    }

    test_validate(dir);
    EXPECT_EQ(test_height(dir), 1);
}

TEST(SimpleList, Hdf5Default) {
    auto dir = define_test_path("simple_list");

    // Still works with an implicit default format of HDF5.
    {
        initialize_directory(dir);
        {
            std::ofstream output(dir / "OBJECT");
            output << "{ \"type\": \"simple_list\", \"simple_list\": { \"version\": \"1.0\" } }";
        }
        H5::H5File handle(dir / "list_contents.h5", H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup("simple_list");
        add_hdf5_attribute(ghandle, "uzuki_object", "list");
        ghandle.createGroup("data");
    }
    test_validate(dir);
    EXPECT_EQ(test_height(dir), 0);
}

TEST(SimpleList, Hdf5Length) {
    auto dir = define_test_path("simple_list");
    initialize_directory(dir);

    // Validates against the length, if supplied.
    {
        std::ofstream output(dir / "OBJECT");
        output << "{ \"type\": \"simple_list\", \"simple_list\": { \"version\": \"1.1\", \"format\": \"hdf5\", \"length\": 3 } }";
        initialize_simple_list_with_metadata(dir, "1.0", "hdf5");
        H5::H5File handle(dir / "list_contents.h5", H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup("simple_list");
        add_hdf5_attribute(ghandle, "uzuki_object", "list");
        auto dhandle = ghandle.createGroup("data");
        {
            auto ghandle = dhandle.createGroup("0");
            add_hdf5_attribute(ghandle, "uzuki_object", "nothing");
        }
        {
            auto ghandle = dhandle.createGroup("1");
            add_hdf5_attribute(ghandle, "uzuki_object", "vector");
            add_hdf5_attribute(ghandle, "uzuki_type", "integer");
            add_hdf5_dataset(ghandle, "data", H5::PredType::NATIVE_INT32, 10);
        }
        {
            auto ghandle = dhandle.createGroup("2");
            add_hdf5_attribute(ghandle, "uzuki_object", "nothing");
        }
    }
    test_validate(dir);
    EXPECT_EQ(test_height(dir), 3);

    // But also ignores it if no length is supplied.
    {
        std::ofstream output(dir / "OBJECT");
        output << "{ \"type\": \"simple_list\", \"simple_list\": { \"version\": \"1.1\"  } }";
    }
    test_validate(dir);
    EXPECT_EQ(test_height(dir), 3);
}

TEST(SimpleList, Hdf5Error) {
    auto dir = define_test_path("simple_list");

    // Check that we actually run through uzuki2's validator.
    {
        initialize_simple_list_with_metadata(dir, "1.0", "hdf5");
        H5::H5File handle(dir / "list_contents.h5", H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup("simple_list");
        add_hdf5_attribute(ghandle, "uzuki_object", "vector");
        add_hdf5_attribute(ghandle, "uzuki_type", "integer");
        add_hdf5_dataset(ghandle, "data", H5::PredType::NATIVE_INT32, 10);
    }
    expect_validation_error(dir, "top-level");
}

/*****************************************/

TEST(SimpleList, GeneralError) {
    auto dir = define_test_path("simple_list");

    initialize_simple_list_with_metadata(dir, "2.0", "whee");
    expect_validation_error(dir, "unsupported version");

    initialize_simple_list_with_metadata(dir, "1.0", "whee");
    expect_validation_error(dir, "unknown format");

    {
        initialize_directory(dir);
        std::ofstream output(dir / "OBJECT");
        output << "{ \"type\": \"simple_list\", \"simple_list\": { \"version\": \"1.0\", \"format\": null } }";
    }
    expect_validation_error(dir, "expected a JSON string");
}

TEST(SimpleList, ExternalError) {
    auto dir = define_test_path("simple_list");
    initialize_simple_list_with_metadata(dir, "1.0", "json.gz");
    dump_compressed_json(dir, "{ \"type\": \"list\", \"values\": [] }");

    auto odir = dir / "other_contents";
    {
        std::ofstream x(odir);
    }
    expect_validation_error(dir, "expected 'other_contents' to be a directory");

    initialize_directory(odir);
    auto dir0 = odir / "asdasd";
    {
        std::ofstream x(dir0);
    }
    expect_validation_error(dir, "expected an external list object at 'other_contents/0'");

    initialize_directory(odir);
    dir0 = odir / "0";
    {
        std::ofstream x(dir0);
    }
    expect_validation_error(dir, "failed to validate external list object at 'other_contents/0'");

    mock_atomic_vector(dir0, 23, AtomicVectorType::INTEGER);
    expect_validation_error(dir, "fewer instances");
}

TEST(SimpleList, LengthError) {
    auto dir = define_test_path("simple_list");

    {
        initialize_directory(dir);
        std::ofstream output(dir / "OBJECT");
        output << "{ \"type\": \"simple_list\", \"simple_list\": { \"version\": \"1.1\", \"format\": \"json.gz\", \"length\": 2 } }";
        dump_compressed_json(dir, "{ \"type\": \"list\", \"values\": [ { \"type\": \"nothing\" } ] }");
    }
    expect_validation_error(dir, "length of the list");
}
