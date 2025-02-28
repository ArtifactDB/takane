#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "data_frame.h"
#include "simple_list.h"
#include "utils.h"

#include "ritsuko/hdf5/vls/vls.hpp"

#include <numeric>
#include <string>
#include <vector>
#include <fstream>

struct Hdf5DataFrameTest : public ::testing::Test {
    Hdf5DataFrameTest() {
        dir = "TEST_data_frame";
        name = "data_frame";
    }

    std::filesystem::path dir;
    std::string name;

    H5::H5File initialize() {
        initialize_directory_simple(dir, "data_frame", "1.0");
        auto path = dir / "basic_columns.h5";
        return H5::H5File(std::string(path), H5F_ACC_TRUNC);
    }

    H5::H5File reopen() {
        auto path = dir / "basic_columns.h5";
        return H5::H5File(path, H5F_ACC_RDWR);
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

TEST_F(Hdf5DataFrameTest, Rownames) {
    std::vector<data_frame::ColumnDetails> columns(1);
    columns.front().name = "WHEE";

    {
        data_frame::mock(dir, 29, columns);
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        data_frame::attach_row_names(ghandle, 29);
    }
    test_validate(dir);
    EXPECT_EQ(test_height(dir), 29);
    std::vector<size_t> expected_dim{ 29, 1 };
    EXPECT_EQ(test_dimensions(dir), expected_dim);

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        ghandle.unlink("row_names");
        ghandle.createGroup("row_names");
    }
    expect_error("expected a 'row_names' dataset");

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        ghandle.unlink("row_names");
        ghandle.createDataSet("row_names", H5::PredType::NATIVE_INT, H5S_SCALAR);
    }
    expect_error("represented by a UTF-8 encoded string");

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        ghandle.unlink("row_names");
        data_frame::attach_row_names(ghandle, 20);
    }
    expect_error("expected 'row_names' to have length");
}

TEST_F(Hdf5DataFrameTest, Colnames) {
    std::vector<data_frame::ColumnDetails> columns(2);
    columns[0].name = "Aaron";
    columns[1].name = "Barry";

    data_frame::mock(dir, 29, columns);
    test_validate(dir);
    EXPECT_EQ(test_height(dir), 29);
    std::vector<size_t> expected_dim{ 29, 2 };
    EXPECT_EQ(test_dimensions(dir), expected_dim);

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        ghandle.unlink("column_names");
    }
    expect_error("dataset");

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        ghandle.createGroup("column_names");
    }
    expect_error("dataset");

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        ghandle.unlink("column_names");
        ghandle.createDataSet("column_names", H5::PredType::NATIVE_INT, H5S_SCALAR);
    }
    expect_error("represented by a UTF-8 encoded string");

    columns[1].name = "Aaron";
    data_frame::mock(dir, 29, columns);
    expect_error("duplicated column name");

    columns[0].name = "";
    data_frame::mock(dir, 29, columns);
    expect_error("empty strings");
}

TEST_F(Hdf5DataFrameTest, General) {
    std::vector<data_frame::ColumnDetails> columns(2);
    columns.resize(2);
    columns[0].name = "Aaron";
    columns[1].name = "Barry";

    H5::StrType stype(0, H5T_VARIABLE);

    initialize_directory_simple(dir, "data_frame", "2.0");
    expect_error("unsupported version");

    {
        auto handle = initialize();
        auto ghandle = handle.createGroup(name);
        ghandle.createAttribute("row-count", H5::PredType::NATIVE_INT8, H5S_SCALAR);
    }
    expect_error("64-bit unsigned");
}

TEST_F(Hdf5DataFrameTest, Data) {
    std::vector<data_frame::ColumnDetails> columns(2);
    columns[0].name = "Aaron";
    columns[1].name = "Barry";

    {
        data_frame::mock(dir, 33, columns);
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        auto dhandle = ghandle.openGroup("data");
        dhandle.unlink("0");
        auto fhandle = dhandle.createGroup("0");
        hdf5_utils::attach_attribute(fhandle, "type", "something");
    }
    expect_error("unsupported type");

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        auto dhandle = ghandle.openGroup("data");
        dhandle.unlink("0");
        hdf5_utils::spawn_data(dhandle, "0", 2, H5::PredType::NATIVE_INT32);
    }
    expect_error("length equal to the number of rows");

    {
        data_frame::mock(dir, 33, columns);
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        auto dhandle = ghandle.openGroup("data");
        dhandle.createGroup("foo");
    }
    expect_error("more objects present");
}

TEST_F(Hdf5DataFrameTest, Other) {
    std::vector<data_frame::ColumnDetails> columns(2);
    columns[0].name = "Aaron";
    columns[0].type = data_frame::ColumnType::OTHER;
    columns[1].name = "Barry";
    columns[1].type = data_frame::ColumnType::OTHER;

    {
        data_frame::mock(dir, 51, columns);

        std::filesystem::create_directory(dir / "other_columns");
        for (size_t i = 0; i < 2; ++i) {
            auto subdir = dir / "other_columns" / std::to_string(i);
            initialize_directory_simple(subdir, "data_frame", "1.0");

            std::vector<data_frame::ColumnDetails> subcolumns(1);
            subcolumns[0].name = "version" + std::to_string(i + 1);
            H5::H5File handle(subdir / "basic_columns.h5", H5F_ACC_TRUNC);
            auto ghandle = handle.createGroup(name);
            data_frame::mock(ghandle, 51, subcolumns);
        }
    }
    test_validate(dir);

    auto subdir = dir / "other_columns" / "0";
    {
        std::vector<data_frame::ColumnDetails> subcolumns(1);
        subcolumns[0].name = "version3";
        data_frame::mock(subdir, 32, subcolumns);
    }
    expect_error("height of column 0 of class 'data_frame'");

    {
        std::filesystem::remove(subdir / "basic_columns.h5");
    }
    expect_error("failed to validate 'other' column 0");

    {
        data_frame::mock(subdir, 51, {});
        data_frame::mock(dir / "other_columns" / "foobar", 51, {});
    }
    expect_error("more objects than expected");
}

TEST_F(Hdf5DataFrameTest, Integer) {
    std::vector<data_frame::ColumnDetails> columns(1);
    columns[0].name = "Aaron";
    columns[0].type = data_frame::ColumnType::INTEGER;

    data_frame::mock(dir, 33, columns);
    test_validate(dir);

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        auto dhandle = ghandle.openGroup("data");
        dhandle.unlink("0");
        auto xhandle = hdf5_utils::spawn_data(dhandle, "0", 33, H5::PredType::NATIVE_INT64);
        hdf5_utils::attach_attribute(xhandle, "type", "integer");
    }
    expect_error("32-bit signed integer");

    // Checking the missing value placeholder.
    {
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        auto dhandle = ghandle.openGroup("data");
        dhandle.unlink("0");
        auto xhandle = hdf5_utils::spawn_data(dhandle, "0", 33, H5::PredType::NATIVE_INT16);
        hdf5_utils::attach_attribute(xhandle, "type", "integer");
        xhandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_INT16, H5S_SCALAR);
    }
    test_validate(dir);

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        auto dhandle = ghandle.openGroup("data");
        auto xhandle = dhandle.openDataSet("0");
        xhandle.removeAttr("missing-value-placeholder");
        xhandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_INT8, H5S_SCALAR);
    }
    expect_error("same type as");
}

TEST_F(Hdf5DataFrameTest, Boolean) {
    std::vector<data_frame::ColumnDetails> columns(1);
    columns[0].name = "Aaron";
    columns[0].type = data_frame::ColumnType::BOOLEAN;

    data_frame::mock(dir, 55, columns);
    test_validate(dir);

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        auto dhandle = ghandle.openGroup("data");
        dhandle.unlink("0");
        auto xhandle = hdf5_utils::spawn_data(dhandle, "0", 55, H5::PredType::NATIVE_INT64);
        hdf5_utils::attach_attribute(xhandle, "type", "boolean");
    }
    expect_error("32-bit signed integer");

    // Checking the missing value placeholder.
    {
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        auto dhandle = ghandle.openGroup("data");
        dhandle.unlink("0");
        auto xhandle = hdf5_utils::spawn_data(dhandle, "0", 55, H5::PredType::NATIVE_INT8);
        hdf5_utils::attach_attribute(xhandle, "type", "boolean");
        xhandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_INT8, H5S_SCALAR);
    }
    test_validate(dir);

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        auto dhandle = ghandle.openGroup("data");
        auto xhandle = dhandle.openDataSet("0");
        xhandle.removeAttr("missing-value-placeholder");
        xhandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_UINT16, H5S_SCALAR);
    }
    expect_error("same type as");
}

TEST_F(Hdf5DataFrameTest, Number) {
    std::vector<data_frame::ColumnDetails> columns(1);
    columns[0].name = "Aaron";
    columns[0].type = data_frame::ColumnType::NUMBER;

    data_frame::mock(dir, 99, columns);
    test_validate(dir);

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        auto dhandle = ghandle.openGroup("data");
        dhandle.unlink("0");
        auto xhandle = hdf5_utils::spawn_data(dhandle, "0", 99, H5::PredType::NATIVE_INT64);
        hdf5_utils::attach_attribute(xhandle, "type", "number");
    }
    expect_error("64-bit float");

    // Checking the missing value placeholder.
    {
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        auto dhandle = ghandle.openGroup("data");
        dhandle.unlink("0");
        auto xhandle = hdf5_utils::spawn_data(dhandle, "0", 99, H5::PredType::NATIVE_DOUBLE);
        hdf5_utils::attach_attribute(xhandle, "type", "number");
        xhandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_DOUBLE, H5S_SCALAR);
    }
    test_validate(dir);

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        auto dhandle = ghandle.openGroup("data");
        auto xhandle = dhandle.openDataSet("0");
        xhandle.removeAttr("missing-value-placeholder");
        xhandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_INT8, H5S_SCALAR);
    }
    expect_error("same type as");
}

TEST_F(Hdf5DataFrameTest, String) {
    std::vector<data_frame::ColumnDetails> columns(1);
    columns[0].name = "Aaron";
    columns[0].type = data_frame::ColumnType::STRING;

    {
        auto handle = initialize();
        auto ghandle = handle.createGroup(name);
        mock(ghandle, 72, columns);
    }
    test_validate(dir);

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        auto dhandle = ghandle.openGroup("data");
        dhandle.unlink("0");
        auto xhandle = hdf5_utils::spawn_data(dhandle, "0", 72, H5::PredType::NATIVE_INT);
        hdf5_utils::attach_attribute(xhandle, "type", "string");
    }
    expect_error("represented by a UTF-8 encoded string");

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        auto dhandle = ghandle.openGroup("data");
        dhandle.unlink("0");
        auto xhandle = hdf5_utils::spawn_data(dhandle, "0", 72, H5::StrType(0, 5));
        hdf5_utils::attach_attribute(xhandle, "type", "string");
        hdf5_utils::attach_attribute(xhandle, "format", "whee");
    }
    expect_error("unsupported format 'whee'");

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        auto dhandle = ghandle.openGroup("data");
        auto xhandle = dhandle.openDataSet("0");
        xhandle.removeAttr("format");
        hdf5_utils::attach_attribute(xhandle, "format", "none");
    }
    test_validate(dir);

    // Checking the missing value placeholder.
    {
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        auto dhandle = ghandle.openGroup("data");
        auto xhandle = dhandle.openDataSet("0");
        H5::StrType stype(0, H5T_VARIABLE);
        auto ahandle = xhandle.createAttribute("missing-value-placeholder", stype, H5S_SCALAR);
        ahandle.write(stype, std::string("asdasd"));
    }
    test_validate(dir);

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        auto dhandle = ghandle.openGroup("data");
        auto xhandle = dhandle.openDataSet("0");
        xhandle.removeAttr("missing-value-placeholder");
        xhandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_INT8, H5S_SCALAR);
    }
    expect_error("same type class as");
}

TEST_F(Hdf5DataFrameTest, StringFormat) {
    std::vector<data_frame::ColumnDetails> columns(1);
    columns[0].name = "Aaron";
    columns[0].type = data_frame::ColumnType::STRING;

    {
        auto handle = initialize();
        auto ghandle = handle.createGroup(name);
        mock(ghandle, 72, columns);
        auto dhandle = ghandle.openGroup("data");
        auto xhandle = dhandle.openDataSet("0");
        hdf5_utils::attach_attribute(xhandle, "format", "date-time");
    }
    expect_error("date/time-formatted string");

    // But it's okay when we slap a placeholder on top.
    {
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        auto dhandle = ghandle.openGroup("data");
        dhandle.unlink("0");
        auto xhandle = hdf5_utils::spawn_data(dhandle, "0", 72, H5::StrType(0, 5));
        hdf5_utils::attach_attribute(xhandle, "type", "string");
        hdf5_utils::attach_attribute(xhandle, "missing-value-placeholder", "");
    }
    test_validate(dir);
}

TEST_F(Hdf5DataFrameTest, Factor) {
    std::vector<data_frame::ColumnDetails> columns(1);
    columns[0].name = "Aaron";
    columns[0].type = data_frame::ColumnType::FACTOR;
    columns[0].factor_levels = std::vector<std::string>{ "kanon", "chisato", "sumire", "ren", "keke" };

    {
        auto handle = initialize();
        auto ghandle = handle.createGroup(name);
        mock(ghandle, 99, columns);
    }
    test_validate(dir);

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        auto dhandle = ghandle.openGroup("data");
        auto fhandle = dhandle.openGroup("0");
        fhandle.unlink("codes");
        hdf5_utils::spawn_data(fhandle, "codes", 80, H5::PredType::NATIVE_UINT8);
    }
    expect_error("length equal to the number of rows");

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        auto dhandle = ghandle.openGroup("data");
        auto fhandle = dhandle.openGroup("0");
        fhandle.unlink("codes");

        std::vector<int> replacement(99, columns[0].factor_levels.size());
        auto xhandle = hdf5_utils::spawn_data(fhandle, "codes", replacement.size(), H5::PredType::NATIVE_UINT16);
        xhandle.write(replacement.data(), H5::PredType::NATIVE_INT);
    }
    expect_error("less than the number of levels");

    {
        auto handle = initialize();
        auto ghandle = handle.createGroup(name);
        mock(ghandle, 99, columns);

        auto dhandle = ghandle.openGroup("data");
        auto fhandle = dhandle.openGroup("0");
        fhandle.unlink("levels");

        std::vector<std::string> levels(columns[0].factor_levels.begin(), columns[0].factor_levels.end());
        levels.push_back(levels[0]);
        hdf5_utils::spawn_string_data(fhandle, "levels", H5T_VARIABLE, levels);
    }
    expect_error("duplicated factor level");

    {
        auto handle = initialize();
        auto ghandle = handle.createGroup(name);
        mock(ghandle, 99, columns);
        auto fhandle = ghandle.openGroup("data/0");
        fhandle.createAttribute("ordered", H5::PredType::NATIVE_FLOAT, H5S_SCALAR);
    }
    expect_error("32-bit signed integer");

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        auto fhandle = ghandle.openGroup("data/0");
        fhandle.removeAttr("ordered");
        fhandle.createAttribute("ordered", H5::PredType::NATIVE_UINT8, H5S_SCALAR);
    }
    test_validate(dir);
}

TEST_F(Hdf5DataFrameTest, Metadata) {
    std::vector<data_frame::ColumnDetails> columns(1);
    columns[0].name = "Aaron";
    columns[0].type = data_frame::ColumnType::FACTOR;
    columns[0].factor_levels = std::vector<std::string>{ "kanon", "chisato", "sumire", "ren", "keke" };

    auto cdir = dir / "column_annotations";
    auto odir = dir / "other_annotations";

    data_frame::mock(dir, 99, columns);
    initialize_directory_simple(cdir, "simple_list", "1.0");
    expect_error("'DATA_FRAME'"); 

    data_frame::mock(cdir, columns.size(), {});
    initialize_directory_simple(odir, "data_frame", "1.0");
    expect_error("'SIMPLE_LIST'");

    simple_list::mock(odir);
    test_validate(dir);
}

TEST_F(Hdf5DataFrameTest, Vls) {
    std::string heap = "abcdefghijklmno";
    hsize_t nrows = 10;

    {
        initialize_directory_simple(dir, "data_frame", "1.1");
        auto path = dir / "basic_columns.h5";
        H5::H5File handle(std::string(path), H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup(name);
        std::vector<data_frame::ColumnDetails> columns(1);
        columns[0].name = "superstring";
        data_frame::mock(ghandle, nrows, columns);

        auto xhandle = ghandle.openGroup("data");
        xhandle.unlink("0");
        auto vhandle = xhandle.createGroup("0");
        hdf5_utils::attach_attribute(vhandle, "type", "vls");

        const unsigned char* hptr = reinterpret_cast<const unsigned char*>(heap.c_str());
        hsize_t hlen = heap.size();
        H5::DataSpace hspace(1, &hlen);
        auto hhandle = vhandle.createDataSet("heap", H5::PredType::NATIVE_UINT8, hspace);
        hhandle.write(hptr, H5::PredType::NATIVE_UCHAR);

        std::vector<ritsuko::hdf5::vls::Pointer<uint64_t, uint64_t> > pointers(nrows);
        for (size_t i = 0; i < nrows; ++i) {
            pointers[i].offset = i; 
            pointers[i].length = 1;
        }
        H5::DataSpace pspace(1, &nrows);
        auto ptype = ritsuko::hdf5::vls::define_pointer_datatype<uint64_t, uint64_t>();
        auto phandle = vhandle.createDataSet("pointers", ptype, pspace);
        phandle.write(pointers.data(), ptype);
    }

    test_validate(dir);

    // Adding a missing value placeholder.
    {
        {
            auto handle = reopen();
            auto dhandle = handle.openDataSet("data_frame/data/0/pointers");
            dhandle.createAttribute("missing-value-placeholder", H5::StrType(0, 10), H5S_SCALAR);
        }
        test_validate(dir);

        // Adding the wrong missing value placeholder.
        {
            auto handle = reopen();
            auto dhandle = handle.openDataSet("data_frame/data/0/pointers");
            dhandle.removeAttr("missing-value-placeholder");
            dhandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_INT, H5S_SCALAR);
        }
        expect_error("same type class");

        // Removing for the next checks.
        {
            auto handle = reopen();
            auto dhandle = handle.openDataSet("data_frame/data/0/pointers");
            dhandle.removeAttr("missing-value-placeholder");
        }
    }

    // Checking that mismatches in the number of rows is detected.
    {
        {
            auto handle = reopen();
            auto dhandle = handle.openDataSet("data_frame/data/0/pointers");
            dhandle.createAttribute("missing-value-placeholder", H5::StrType(0, 10), H5S_SCALAR);
        }
        test_validate(dir);
    }

    // Checking that this only works in the latest version.
    {
        auto opath = dir/"OBJECT";
        auto parsed = millijson::parse_file(opath.c_str());
        auto& entries = reinterpret_cast<millijson::Object*>(parsed.get())->values;
        auto& df_entries = reinterpret_cast<millijson::Object*>(entries["data_frame"].get())->values;
        reinterpret_cast<millijson::String*>(df_entries["version"].get())->value = "1.0";
        json_utils::dump(parsed.get(), opath);

        expect_error("unsupported type");

        reinterpret_cast<millijson::String*>(df_entries["version"].get())->value = "1.1";
        json_utils::dump(parsed.get(), opath);
    }

    // Shortening the heap to check that we perform bounds checks on the pointers.
    {
        {
            auto handle = reopen();
            auto vhandle = handle.openGroup("data_frame/data/0");
            vhandle.unlink("heap");
            hsize_t zero = 0;
            H5::DataSpace hspace(1, &zero);
            vhandle.createDataSet("heap", H5::PredType::NATIVE_UINT8, hspace);
        }
        expect_error("out of range");
    }

    // Checking that we check for 64-bit unsigned integer types. 
    {
        {
            auto handle = reopen();
            auto vhandle = handle.openGroup("data_frame/data/0");
            vhandle.unlink("pointers");

            std::vector<ritsuko::hdf5::vls::Pointer<int, int> > pointers(nrows);
            for (auto& pp : pointers) {
                pp.offset = 0;
                pp.length = 0;
            }
            H5::DataSpace pspace(1, &nrows);
            auto ptype = ritsuko::hdf5::vls::define_pointer_datatype<int, int>();
            auto phandle = vhandle.createDataSet("pointers", ptype, pspace);
            phandle.write(pointers.data(), ptype);
        }
        expect_error("64-bit unsigned integer");
    }

    // Checking that we check for 64-bit unsigned integer types. 
    {
        {
            auto handle = reopen();
            auto vhandle = handle.openGroup("data_frame/data/0");
            vhandle.unlink("pointers");

            hsize_t nrows_p1 = nrows + 1;
            std::vector<ritsuko::hdf5::vls::Pointer<uint8_t, uint8_t> > pointers(nrows_p1);
            for (auto& pp : pointers) {
                pp.offset = 0;
                pp.length = 0;
            }
            H5::DataSpace pspace(1, &nrows_p1);
            auto ptype = ritsuko::hdf5::vls::define_pointer_datatype<uint8_t, uint8_t>();
            auto phandle = vhandle.createDataSet("pointers", ptype, pspace);
            phandle.write(pointers.data(), ptype);
        }
        expect_error("number of rows");
    }
}
