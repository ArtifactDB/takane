#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "data_frame.h"
#include "takane/data_frame.hpp"
#include "takane/validate.hpp"
#include "takane/HEIGHT.hpp"

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
        if (std::filesystem::exists(dir)) {
            std::filesystem::remove_all(dir);
        }
        std::filesystem::create_directory(dir);

        auto path = dir / "basic_columns.h5";
        return H5::H5File(std::string(path), H5F_ACC_TRUNC);
    }

    H5::H5File reopen() {
        auto path = dir / "basic_columns.h5";
        return H5::H5File(path, H5F_ACC_RDWR);
    }

    template<typename ... Args_>
    void expect_error(const std::string& msg, Args_&& ... args) {
        EXPECT_ANY_THROW({
            try {
                takane::data_frame::validate(dir, std::forward<Args_>(args)...);
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
        auto handle = initialize();
        auto ghandle = handle.createGroup(name);
        data_frame::mock(ghandle, 29, true, columns);
    }
    takane::data_frame::validate(dir);

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
    expect_error("string dataset");

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        ghandle.unlink("row_names");

        H5::StrType stype(0, H5T_VARIABLE);
        hsize_t dummy = 20;
        H5::DataSpace dspace(1, &dummy);
        ghandle.createDataSet("row_names", stype, dspace);
    }
    expect_error("expected 'row_names' to have length");
}

TEST_F(Hdf5DataFrameTest, Colnames) {
    std::vector<data_frame::ColumnDetails> columns(2);
    columns[0].name = "Aaron";
    columns[1].name = "Barry";

    {
        auto handle = initialize();
        auto ghandle = handle.createGroup(name);
        mock(ghandle, 29, false, columns);
    }
    takane::data_frame::validate(dir);
    
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
    expect_error("string dataset");

    columns[1].name = "Aaron";
    {
        auto handle = initialize();
        auto ghandle = handle.createGroup(name);
        mock(ghandle, 29, false, columns);
    }
    expect_error("duplicated column name");

    columns[0].name = "";
    {
        auto handle = initialize();
        auto ghandle = handle.createGroup(name);
        mock(ghandle, 29, false, columns);
    }
    expect_error("empty strings");
}

TEST_F(Hdf5DataFrameTest, General) {
    std::vector<data_frame::ColumnDetails> columns(2);
    columns.resize(2);
    columns[0].name = "Aaron";
    columns[1].name = "Barry";

    {
        initialize();
    }
    expect_error("'" + name + "' group");

    H5::StrType stype(0, H5T_VARIABLE);
    {
        auto handle = reopen();
        auto ghandle = handle.createGroup(name);
        auto attr = ghandle.createAttribute("version", stype, H5S_SCALAR);
        attr.write(stype, std::string("2.0"));
    }
    expect_error("unsupported version");

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        ghandle.removeAttr("version");
        auto attr = ghandle.createAttribute("version", stype, H5S_SCALAR);
        attr.write(stype, std::string("1.0"));
        ghandle.createAttribute("row-count", H5::PredType::NATIVE_INT8, H5S_SCALAR);
    }
    expect_error("64-bit unsigned");
}

TEST_F(Hdf5DataFrameTest, Data) {
    std::vector<data_frame::ColumnDetails> columns(2);
    columns[0].name = "Aaron";
    columns[1].name = "Barry";

    {
        auto handle = initialize();
        auto ghandle = handle.createGroup(name);
        mock(ghandle, 33, false, columns);
        ghandle.unlink("data");
    }
    expect_error("'data_frame/data' group");

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        auto dhandle = ghandle.createGroup("data");
        auto fhandle = dhandle.createGroup("0");
        Hdf5Utils::attach_attribute(fhandle, "type", "something");
    }
    expect_error("expected HDF5 groups");

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        auto dhandle = ghandle.openGroup("data");
        dhandle.unlink("0");
        Hdf5Utils::spawn_data(dhandle, "0", 2, H5::PredType::NATIVE_INT32);
    }
    expect_error("length equal to the number of rows");

    {
        auto handle = initialize();
        auto ghandle = handle.createGroup(name);
        mock(ghandle, 33, false, columns);
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
        auto handle = initialize();
        auto ghandle = handle.createGroup(name);
        mock(ghandle, 51, false, columns);

        std::filesystem::create_directory(dir / "other_columns");
        for (size_t i = 0; i < 2; ++i) {
            auto subdir = dir / "other_columns" / std::to_string(i);
            std::filesystem::create_directory(subdir);
            std::ofstream output(subdir / "OBJECT");
            output << "data_frame";

            std::vector<data_frame::ColumnDetails> subcolumns(1);
            subcolumns[0].name = "version" + std::to_string(i + 1);
            H5::H5File handle(subdir / "basic_columns.h5", H5F_ACC_TRUNC);
            auto ghandle = handle.createGroup(name);
            mock(ghandle, 51, false, subcolumns);
        }
    }
    takane::data_frame::validate(dir);

    {
        std::filesystem::create_directory(dir / "other_columns");
        auto subdir = dir / "other_columns" / "0";
        std::vector<data_frame::ColumnDetails> subcolumns(1);
        subcolumns[0].name = "version3";
        H5::H5File handle(subdir / "basic_columns.h5", H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup(name);
        mock(ghandle, 32, false, subcolumns);
    }
    expect_error("height of column of class 'data_frame'");
}

TEST_F(Hdf5DataFrameTest, Integer) {
    std::vector<data_frame::ColumnDetails> columns(1);
    columns[0].name = "Aaron";
    columns[0].type = data_frame::ColumnType::INTEGER;

    {
        auto handle = initialize();
        auto ghandle = handle.createGroup(name);
        data_frame::mock(ghandle, 33, false, columns);
    }
    takane::data_frame::validate(dir);

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        auto dhandle = ghandle.openGroup("data");
        dhandle.unlink("0");
        auto xhandle = Hdf5Utils::spawn_data(dhandle, "0", 33, H5::PredType::NATIVE_INT64);
        Hdf5Utils::attach_attribute(xhandle, "type", "integer");
    }
    expect_error("32-bit signed integer");

    // Checking the missing value placeholder.
    {
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        auto dhandle = ghandle.openGroup("data");
        dhandle.unlink("0");
        auto xhandle = Hdf5Utils::spawn_data(dhandle, "0", 33, H5::PredType::NATIVE_INT16);
        Hdf5Utils::attach_attribute(xhandle, "type", "integer");
        xhandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_INT16, H5S_SCALAR);
    }
    takane::data_frame::validate(dir);

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

    {
        auto handle = initialize();
        auto ghandle = handle.createGroup(name);
        data_frame::mock(ghandle, 55, false, columns);
    }
    takane::data_frame::validate(dir);

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        auto dhandle = ghandle.openGroup("data");
        dhandle.unlink("0");
        auto xhandle = Hdf5Utils::spawn_data(dhandle, "0", 55, H5::PredType::NATIVE_INT64);
        Hdf5Utils::attach_attribute(xhandle, "type", "boolean");
    }
    expect_error("32-bit signed integer");

    // Checking the missing value placeholder.
    {
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        auto dhandle = ghandle.openGroup("data");
        dhandle.unlink("0");
        auto xhandle = Hdf5Utils::spawn_data(dhandle, "0", 55, H5::PredType::NATIVE_INT8);
        Hdf5Utils::attach_attribute(xhandle, "type", "boolean");
        xhandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_INT8, H5S_SCALAR);
    }
    takane::data_frame::validate(dir);

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

    {
        auto handle = initialize();
        auto ghandle = handle.createGroup(name);
        data_frame::mock(ghandle, 99, false, columns);
    }
    takane::data_frame::validate(dir);

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        auto dhandle = ghandle.openGroup("data");
        dhandle.unlink("0");
        auto xhandle = Hdf5Utils::spawn_data(dhandle, "0", 99, H5::PredType::NATIVE_INT64);
        Hdf5Utils::attach_attribute(xhandle, "type", "number");
    }
    expect_error("64-bit float");

    // Checking the missing value placeholder.
    {
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        auto dhandle = ghandle.openGroup("data");
        dhandle.unlink("0");
        auto xhandle = Hdf5Utils::spawn_data(dhandle, "0", 99, H5::PredType::NATIVE_DOUBLE);
        Hdf5Utils::attach_attribute(xhandle, "type", "number");
        xhandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_DOUBLE, H5S_SCALAR);
    }
    takane::data_frame::validate(dir);

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
        mock(ghandle, 72, false, columns);
    }
    takane::data_frame::validate(dir);

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        auto dhandle = ghandle.openGroup("data");
        dhandle.unlink("0");
        auto xhandle = Hdf5Utils::spawn_data(dhandle, "0", 72, H5::PredType::NATIVE_INT);
        Hdf5Utils::attach_attribute(xhandle, "type", "string");
    }
    expect_error("string dataset");

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        auto dhandle = ghandle.openGroup("data");
        dhandle.unlink("0");
        auto xhandle = Hdf5Utils::spawn_data(dhandle, "0", 72, H5::StrType(0, 5));
        Hdf5Utils::attach_attribute(xhandle, "type", "string");
        Hdf5Utils::attach_attribute(xhandle, "format", "whee");
    }
    expect_error("unsupported format 'whee'");

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        auto dhandle = ghandle.openGroup("data");
        auto xhandle = dhandle.openDataSet("0");
        xhandle.removeAttr("format");
        Hdf5Utils::attach_attribute(xhandle, "format", "none");
    }
    takane::data_frame::validate(dir);

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
    takane::data_frame::validate(dir);

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
        mock(ghandle, 72, false, columns);
        auto dhandle = ghandle.openGroup("data");
        auto xhandle = dhandle.openDataSet("0");
        Hdf5Utils::attach_attribute(xhandle, "format", "date-time");
    }
    expect_error("date/time-formatted string");

    // But it's okay when we slap a placeholder on top.
    {
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        auto dhandle = ghandle.openGroup("data");
        dhandle.unlink("0");
        auto xhandle = Hdf5Utils::spawn_data(dhandle, "0", 72, H5::StrType(0, 5));
        Hdf5Utils::attach_attribute(xhandle, "type", "string");
        Hdf5Utils::attach_attribute(xhandle, "missing-value-placeholder", "");
    }
    takane::data_frame::validate(dir);
}

TEST_F(Hdf5DataFrameTest, Factor) {
    std::vector<data_frame::ColumnDetails> columns(1);
    columns[0].name = "Aaron";
    columns[0].type = data_frame::ColumnType::FACTOR;
    columns[0].factor_levels = std::vector<std::string>{ "kanon", "chisato", "sumire", "ren", "keke" };

    {
        auto handle = initialize();
        auto ghandle = handle.createGroup(name);
        mock(ghandle, 99, false, columns);
    }
    takane::data_frame::validate(dir);

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        auto dhandle = ghandle.openGroup("data");
        auto fhandle = dhandle.openGroup("0");
        fhandle.unlink("codes");
        Hdf5Utils::spawn_data(fhandle, "codes", 80, H5::PredType::NATIVE_INT8);
    }
    expect_error("length equal to the number of rows");

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        auto dhandle = ghandle.openGroup("data");
        auto fhandle = dhandle.openGroup("0");
        fhandle.unlink("codes");

        std::vector<int> replacement(99, columns[0].factor_levels.size());
        auto xhandle = Hdf5Utils::spawn_data(fhandle, "codes", replacement.size(), H5::PredType::NATIVE_INT16);
        xhandle.write(replacement.data(), H5::PredType::NATIVE_INT);
    }
    expect_error("less than the number of levels");

    {
        auto handle = initialize();
        auto ghandle = handle.createGroup(name);
        mock(ghandle, 99, false, columns);

        auto dhandle = ghandle.openGroup("data");
        auto fhandle = dhandle.openGroup("0");
        fhandle.unlink("levels");

        std::vector<std::string> levels(columns[0].factor_levels.begin(), columns[0].factor_levels.end());
        levels.push_back(levels[0]);
        Hdf5Utils::spawn_string_data(fhandle, "levels", H5T_VARIABLE, levels);
    }
    expect_error("duplicate factor level");

    {
        auto handle = initialize();
        auto ghandle = handle.createGroup(name);
        mock(ghandle, 99, false, columns);
        auto fhandle = ghandle.openGroup("data/0");
        fhandle.createAttribute("ordered", H5::PredType::NATIVE_FLOAT, H5S_SCALAR);
    }
    expect_error("32-bit signed integer");

    {
        auto handle = initialize();
        auto ghandle = handle.createGroup(name);
        mock(ghandle, 99, false, columns);
        auto fhandle = ghandle.openGroup("data/0");
        fhandle.removeAttr("ordered");
        fhandle.createAttribute("ordered", H5::PredType::NATIVE_UINT8, H5S_SCALAR);
    }
    takane::data_frame::validate(dir);
}
