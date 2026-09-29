#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "mock_data_frame.h"
#include "mock_simple_list.h"
#include "utils.h"

#include "takane/data_frame.hpp"
#include "ritsuko/ritsuko.hpp"

#include <numeric>
#include <string>
#include <vector>
#include <fstream>

TEST(DataFrame, RownamesOkay) {
    auto dir = define_test_path("data_frame");

    std::vector<DataFrameColumnDetails> columns(1);
    columns.front().name = "WHEE";

    {
        auto ghandle = mock_data_frame(dir, 29, columns);
        attach_row_names_to_data_frame(ghandle, 29);
    }

    test_validate(dir);
    EXPECT_EQ(test_height(dir), 29);
    std::vector<std::size_t> expected_dim{ 29, 1 };
    EXPECT_EQ(test_dimensions(dir), expected_dim);
}

TEST(DataFrame, RownamesError) {
    auto dir = define_test_path("data_frame");

    std::vector<DataFrameColumnDetails> columns(1);
    columns.front().name = "WHEE";

    {
        auto ghandle = mock_data_frame(dir, 29, columns);
        add_hdf5_dataset(ghandle, "row_names", H5::PredType::NATIVE_INT32, 29);
    }
    expect_validation_error(dir, "represented by a UTF-8 encoded string");

    {
        auto ghandle = mock_data_frame(dir, 29, columns);
        ghandle.createDataSet("row_names", H5::StrType(0, 10), H5S_SCALAR);
    }
    expect_validation_error(dir, "1-dimensional");

    {
        auto ghandle = mock_data_frame(dir, 29, columns);
        add_hdf5_dataset(ghandle, "row_names", H5::StrType(0, 10), 30);
    }
    expect_validation_error(dir, "expected 'row_names' to have length");

    {
        auto ghandle = mock_data_frame(dir, 29, columns);
        add_hdf5_dataset(ghandle, "row_names", H5::StrType(0, H5T_VARIABLE), 29);
    }
    expect_validation_error(dir, "failed to validate");
}

/**************************************/

TEST(DataFrame, ColnamesOkay) {
    auto dir = define_test_path("data_frame");

    std::vector<DataFrameColumnDetails> columns(2);
    columns[0].name = "Aaron";
    columns[1].name = "Barry";

    {
        mock_data_frame(dir, 29, columns);
    }

    test_validate(dir);
    EXPECT_EQ(test_height(dir), 29);
    std::vector<size_t> expected_dim{ 29, 2 };
    EXPECT_EQ(test_dimensions(dir), expected_dim);
}

TEST(DataFrame, ColnamesError) {
    auto dir = define_test_path("data_frame");

    std::vector<DataFrameColumnDetails> columns(3);
    columns[0].name = "Aaron";
    columns[1].name = "Barry";
    columns[2].name = "Charlie";

    {
        auto ghandle = mock_data_frame(dir, 29, columns);
        ghandle.unlink("column_names");
        add_hdf5_dataset(ghandle, "column_names", H5::PredType::NATIVE_INT, columns.size());
    }
    expect_validation_error(dir, "UTF-8 encoded string");

    {
        auto ghandle = mock_data_frame(dir, 29, columns);
        ghandle.unlink("column_names");
        ghandle.createDataSet("column_names", H5::StrType(0, 5), H5S_SCALAR);
    }
    expect_validation_error(dir, "1-dimensional");

    columns[1].name = "Aaron";
    {
        mock_data_frame(dir, 29, columns);
    }
    expect_validation_error(dir, "duplicated column name");

    columns[0].name = "";
    {
        mock_data_frame(dir, 29, columns);
    }
    expect_validation_error(dir, "empty strings");
}

/**************************************/

TEST(DataFrame, MixedOkay) {
    auto dir = define_test_path("data_frame");

    // DataFrame with many different column types works correctly.
    std::vector<DataFrameColumnDetails> columns(6);
    columns[0].name = "Aaron";
    columns[0].type = DataFrameColumnType::BOOLEAN;
    columns[1].name = "Barry";
    columns[1].type = DataFrameColumnType::NUMBER;
    columns[2].name = "Charlie";
    columns[2].type = DataFrameColumnType::STRING;
    columns[3].name = "Delta";
    columns[3].type = DataFrameColumnType::INTEGER;
    columns[4].name = "Echo";
    columns[4].type = DataFrameColumnType::FACTOR;
    columns[4].factor_levels = std::vector<std::string>{ "akari", "aika", "alice", "ai" };
    columns[5].name = "Foxtrot";
    columns[5].type = DataFrameColumnType::FACTOR;
    columns[5].factor_levels = std::vector<std::string>{ "athena", "akira", "alicia" };
    columns[5].factor_ordered = true;

    const hsize_t len = 51;
    {
        mock_data_frame(dir, len, columns);
    }
    {
        test_validate(dir);
        EXPECT_EQ(test_height(dir), len);
        std::vector<std::size_t> expected_dim;
        expected_dim.push_back(len);
        expected_dim.push_back(columns.size());
        EXPECT_EQ(test_dimensions(dir), expected_dim);
    }

    // Injecting a VLS column.
    columns.emplace_back();
    columns[6].name = "Gamma";
    {
        initialize_directory_simple(dir, "data_frame", "1.1");
        H5::H5File handle(dir / "basic_columns.h5", H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup("data_frame");
        mock_data_frame(ghandle, len, columns);
        auto dhandle = ghandle.openGroup("data");
        dhandle.unlink("6");

        auto vhandle = dhandle.createGroup("6");
        add_hdf5_attribute(vhandle, "type", "vls");
        auto vtype = ritsuko::cvls::define_pointer_datatype<std::uint16_t, std::uint16_t>();
        auto phandle = vhandle.createDataSet("pointers", vtype, H5::DataSpace(1, &len));

        std::vector<ritsuko::cvls::Pointer<std::uint16_t, std::uint16_t> > buffer(len);
        hsize_t previous = 0;
        for (hsize_t i = 0; i < len; ++i) {
            buffer[i].offset = previous; 
            const auto len = 1 + i % 10;
            buffer[i].length = len;
            previous += len;
        }
        phandle.write(buffer.data(), vtype);
        
        add_hdf5_dataset(vhandle, "heap", H5::PredType::NATIVE_UINT8, previous);
    }
    {
        test_validate(dir);
        EXPECT_EQ(test_height(dir), len);
        std::vector<std::size_t> expected_dim;
        expected_dim.push_back(len);
        expected_dim.push_back(columns.size());
        EXPECT_EQ(test_dimensions(dir), expected_dim);
    }
}

/**************************************/

TEST(DataFrame, BooleanError) {
    auto dir = define_test_path("data_frame");

    std::vector<DataFrameColumnDetails> columns(1);
    columns[0].name = "Kyoko";
    columns[0].type = DataFrameColumnType::NUMBER;

    {
        auto ghandle = mock_data_frame(dir, 55, columns);
        auto dhandle = ghandle.openGroup("data");
        auto xhandle = dhandle.openDataSet("0");
        xhandle.removeAttr("type");
        add_hdf5_attribute(xhandle, "type", "boolean");
    }
    expect_validation_error(dir, "32-bit signed integer");
}

TEST(DataFrame, IntegerError) {
    auto dir = define_test_path("data_frame");

    std::vector<DataFrameColumnDetails> columns(1);
    columns[0].name = "Kyubey";
    columns[0].type = DataFrameColumnType::NUMBER;

    {
        auto ghandle = mock_data_frame(dir, 55, columns);
        auto dhandle = ghandle.openGroup("data");
        auto xhandle = dhandle.openDataSet("0");
        xhandle.removeAttr("type");
        add_hdf5_attribute(xhandle, "type", "integer");
    }
    expect_validation_error(dir, "32-bit signed integer");
}

TEST(DataFrame, NumberError) {
    auto dir = define_test_path("data_frame");

    std::vector<DataFrameColumnDetails> columns(1);
    columns[0].name = "Madoka";
    columns[0].type = DataFrameColumnType::NUMBER;

    {
        auto ghandle = mock_data_frame(dir, 55, columns);
        auto dhandle = ghandle.openGroup("data");
        dhandle.unlink("0");
        auto xhandle = add_hdf5_dataset(dhandle, "0", H5::PredType::NATIVE_INT64, 55);
        add_hdf5_attribute(xhandle, "type", "number");
    }
    expect_validation_error(dir, "64-bit float");
}

TEST(DataFrame, NumericMissingOkay) {
    auto dir = define_test_path("data_frame");

    std::vector<DataFrameColumnDetails> columns(3);
    columns[0].name = "Madoka";
    columns[0].type = DataFrameColumnType::BOOLEAN;
    columns[1].name = "Homura";
    columns[1].type = DataFrameColumnType::INTEGER;
    columns[2].name = "Sayaka";
    columns[2].type = DataFrameColumnType::NUMBER;

    {
        auto ghandle = mock_data_frame(dir, 55, columns);
        auto dhandle = ghandle.openGroup("data");
        auto x0handle = dhandle.openDataSet("0");
        x0handle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_INT8, H5S_SCALAR);
        auto x1handle = dhandle.openDataSet("1");
        x1handle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_INT32, H5S_SCALAR);
        auto x2handle = dhandle.openDataSet("2");
        x2handle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_DOUBLE, H5S_SCALAR);
    }

    test_validate(dir);
    EXPECT_EQ(test_height(dir), 55);
    std::vector<std::size_t> expected_dim{ 55, 3 };
    EXPECT_EQ(test_dimensions(dir), expected_dim);
}

TEST(DataFrame, NumericMissingError) {
    auto dir = define_test_path("data_frame");

    for (int i = 0; i < 3; ++i) {
        std::vector<DataFrameColumnDetails> columns(1);
        columns[0].name = "mami";
        if (i == 0) {
            columns[0].type = DataFrameColumnType::INTEGER;
        } else if (i == 1) {
            columns[0].type = DataFrameColumnType::BOOLEAN;
        } else {
            columns[0].type = DataFrameColumnType::NUMBER;
        }

        {
            auto ghandle = mock_data_frame(dir, 21, columns);
            auto dhandle = ghandle.openGroup("data");
            auto xhandle = dhandle.openDataSet("0");
            xhandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_INT16, H5S_SCALAR);
        }
        expect_validation_error(dir, "same datatype");
    }
}

/**************************************/

TEST(DataFrame, StringOkay) {
    auto dir = define_test_path("data_frame");

    std::vector<DataFrameColumnDetails> columns(1);
    columns[0].name = "Aaron";
    columns[0].type = DataFrameColumnType::STRING;

    // Injecting a 'none' format to ensure that is respected.
    {
        auto ghandle = mock_data_frame(dir, 32, columns);
        auto dhandle = ghandle.openGroup("data");
        auto xhandle = dhandle.openDataSet("0");
        add_hdf5_attribute(xhandle, "format", "none");
    }

    test_validate(dir);
    EXPECT_EQ(test_height(dir), 32);
    std::vector<std::size_t> expected_dim{ 32, 1 };
    EXPECT_EQ(test_dimensions(dir), expected_dim);
}

TEST(DataFrame, StringError) {
    auto dir = define_test_path("data_frame");

    std::vector<DataFrameColumnDetails> columns(1);
    columns[0].name = "Aaron";
    columns[0].type = DataFrameColumnType::INTEGER;

    {
        auto ghandle = mock_data_frame(dir, 23, columns); 
        auto dhandle = ghandle.openGroup("data");
        auto xhandle = dhandle.openDataSet("0");
        xhandle.removeAttr("type");
        add_hdf5_attribute(xhandle, "type", "string");
    }
    expect_validation_error(dir, "represented by a UTF-8 encoded string");

    // Check that we validate the format.
    columns[0].type = DataFrameColumnType::STRING;
    {
        auto ghandle = mock_data_frame(dir, 23, columns); 
        auto dhandle = ghandle.openGroup("data");
        auto xhandle = dhandle.openDataSet("0");
        const hsize_t len = 12;
        xhandle.createAttribute("format", H5::StrType(0, 10), H5::DataSpace(1, &len));
    }
    expect_validation_error(dir, "scalar");

    {
        auto ghandle = mock_data_frame(dir, 23, columns); 
        auto dhandle = ghandle.openGroup("data");
        auto xhandle = dhandle.openDataSet("0");
        add_hdf5_attribute(xhandle, "format", "foobar"); 
    }
    expect_validation_error(dir, "foobar");

    // Confirm that we validate the string contents.
    columns[0].string_length = H5T_VARIABLE;
    {
        mock_data_frame(dir, 23, columns); 
    }
    expect_validation_error(dir, "NULL");
}

TEST(DataFrame, StringMissingOkay) {
    auto dir = define_test_path("data_frame");

    std::vector<DataFrameColumnDetails> columns(1);
    columns[0].name = "Aaron";
    columns[0].type = DataFrameColumnType::STRING;

    {
        auto ghandle = mock_data_frame(dir, 14, columns); 
        auto dhandle = ghandle.openGroup("data");
        auto xhandle = dhandle.openDataSet("0");
        add_hdf5_attribute(xhandle, "missing-value-placeholder", "asdasd");
    }

    test_validate(dir);
    EXPECT_EQ(test_height(dir), 14);
    std::vector<std::size_t> expected_dim{ 14, 1 };
    EXPECT_EQ(test_dimensions(dir), expected_dim);
}

TEST(DataFrame, StringMissingError) {
    auto dir = define_test_path("data_frame");

    std::vector<DataFrameColumnDetails> columns(1);
    columns[0].name = "Aaron";
    columns[0].type = DataFrameColumnType::STRING;

    {
        auto ghandle = mock_data_frame(dir, 14, columns); 
        auto dhandle = ghandle.openGroup("data");
        auto xhandle = dhandle.openDataSet("0");
        xhandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_INT8, H5S_SCALAR);
    }

    expect_validation_error(dir, "UTF-8 string");
}

TEST(DataFrame, StringFormatOkay) {
    auto dir = define_test_path("data_frame");

    std::vector<DataFrameColumnDetails> columns(1);
    columns[0].name = "Aaron";
    columns[0].type = DataFrameColumnType::STRING;
    columns[0].string_length = H5T_VARIABLE;

    constexpr hsize_t len = 17;
    {
        auto ghandle = mock_data_frame(dir, len, columns);
        auto dhandle = ghandle.openGroup("data");
        auto xhandle = dhandle.openDataSet("0");
        add_hdf5_attribute(xhandle, "format", "date");
        const char* placeholder = "2026-09-29";
        std::vector<const char*> pointers(len, placeholder);
        xhandle.write(pointers.data(), H5::StrType(0, H5T_VARIABLE));
    }

    test_validate(dir);
    EXPECT_EQ(test_height(dir), len);
    std::vector<std::size_t> expected_dim;
    expected_dim.push_back(len);
    expected_dim.push_back(1);
    EXPECT_EQ(test_dimensions(dir), expected_dim);
}

TEST(DataFrame, StringFormatError) {
    auto dir = define_test_path("data_frame");

    std::vector<DataFrameColumnDetails> columns(1);
    columns[0].name = "Aaron";
    columns[0].type = DataFrameColumnType::STRING;
    columns[0].string_length = H5T_VARIABLE;

    constexpr hsize_t len = 9;
    {
        auto ghandle = mock_data_frame(dir, len, columns);
        auto dhandle = ghandle.openGroup("data");
        auto xhandle = dhandle.openDataSet("0");
        add_hdf5_attribute(xhandle, "format", "date");
        const char* placeholder = "2026-09-29";
        const char* dummy = "mitochondria";
        std::vector<const char*> pointers(len, placeholder);
        pointers[len / 2] = dummy;
        xhandle.write(pointers.data(), H5::StrType(0, H5T_VARIABLE));
    }

    expect_validation_error(dir, "date-formatted string");
}

TEST(DataFrame, StringFormatMissingOkay) {
    auto dir = define_test_path("data_frame");

    std::vector<DataFrameColumnDetails> columns(1);
    columns[0].name = "Aaron";
    columns[0].type = DataFrameColumnType::STRING;
    columns[0].string_length = H5T_VARIABLE;

    // We correctly pass along the missing placeholder to the format checker.
    constexpr hsize_t len = 21;
    {
        auto ghandle = mock_data_frame(dir, len, columns);
        auto dhandle = ghandle.openGroup("data");
        auto xhandle = dhandle.openDataSet("0");
        add_hdf5_attribute(xhandle, "format", "date");
        const char* placeholder = "2026-09-29";
        const char* dummy = "mitochondria";
        std::vector<const char*> pointers(len, placeholder);
        pointers[len / 2] = dummy;
        xhandle.write(pointers.data(), H5::StrType(0, H5T_VARIABLE));
        add_hdf5_attribute(xhandle, "missing-value-placeholder", dummy);
    }

    test_validate(dir);
    EXPECT_EQ(test_height(dir), len);
    std::vector<std::size_t> expected_dim;
    expected_dim.push_back(len);
    expected_dim.push_back(1);
    EXPECT_EQ(test_dimensions(dir), expected_dim);
}

/**************************************/

TEST(DataFrame, GeneralError) {
    auto dir = define_test_path("data_frame");

    {
        initialize_directory_simple(dir, "data_frame", "2.0");
    }
    expect_validation_error(dir, "unsupported version");

    {
        initialize_directory_simple(dir, "data_frame", "1.0");
        H5::H5File handle(dir / "basic_columns.h5", H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup("data_frame");
        const hsize_t len = 1;
        ghandle.createAttribute("row-count", H5::PredType::NATIVE_UINT8, H5::DataSpace(1, &len));
    }
    expect_validation_error(dir, "scalar");

    {
        initialize_directory_simple(dir, "data_frame", "1.0");
        H5::H5File handle(dir / "basic_columns.h5", H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup("data_frame");
        ghandle.createAttribute("row-count", H5::PredType::NATIVE_INT8, H5S_SCALAR);
    }
    expect_validation_error(dir, "64-bit unsigned");
}

TEST(DataFrame, DataError) {
    auto dir = define_test_path("data_frame");

    std::vector<DataFrameColumnDetails> columns(2);
    columns[0].name = "Aaron";
    columns[1].name = "Barry";

    {
        auto ghandle = mock_data_frame(dir, 33, columns);
        auto dhandle = ghandle.openGroup("data");
        dhandle.unlink("0");
        auto fhandle = dhandle.createGroup("0");
        add_hdf5_attribute(fhandle, "type", "something");
    }
    expect_validation_error(dir, "unsupported type");

    {
        auto ghandle = mock_data_frame(dir, 33, columns);
        auto dhandle = ghandle.openGroup("data");
        dhandle.unlink("0");
        auto fhandle = add_hdf5_dataset(dhandle, "0", H5::PredType::NATIVE_INT32, 2);
        add_hdf5_attribute(fhandle, "type", "integer");
    }
    expect_validation_error(dir, "not equal to the number of rows");

    {
        auto ghandle = mock_data_frame(dir, 33, columns);
        auto dhandle = ghandle.openGroup("data");
        dhandle.createGroup("foo");
    }
    expect_validation_error(dir, "more objects present");
}

/**************************************/


//TEST_F(Hdf5DataFrameTest, Other) {
//    std::vector<data_frame::ColumnDetails> columns(2);
//    columns[0].name = "Aaron";
//    columns[0].type = data_frame::ColumnType::OTHER;
//    columns[1].name = "Barry";
//    columns[1].type = data_frame::ColumnType::OTHER;
//
//    {
//        data_frame::mock(dir, 51, columns);
//
//        std::filesystem::create_directory(dir / "other_columns");
//        for (size_t i = 0; i < 2; ++i) {
//            auto subdir = dir / "other_columns" / std::to_string(i);
//            initialize_directory_simple(subdir, "data_frame", "1.0");
//
//            std::vector<data_frame::ColumnDetails> subcolumns(1);
//            subcolumns[0].name = "version" + std::to_string(i + 1);
//            H5::H5File handle(subdir / "basic_columns.h5", H5F_ACC_TRUNC);
//            auto ghandle = handle.createGroup(name);
//            data_frame::mock(ghandle, 51, subcolumns);
//        }
//    }
//    test_validate(dir);
//
//    auto subdir = dir / "other_columns" / "0";
//    {
//        std::vector<data_frame::ColumnDetails> subcolumns(1);
//        subcolumns[0].name = "version3";
//        data_frame::mock(subdir, 32, subcolumns);
//    }
//    expect_error("height of column 0 of class 'data_frame'");
//
//    {
//        std::filesystem::remove(subdir / "basic_columns.h5");
//    }
//    expect_error("failed to validate 'other' column 0");
//
//    {
//        data_frame::mock(subdir, 51, {});
//        data_frame::mock(dir / "other_columns" / "foobar", 51, {});
//    }
//    expect_error("more objects than expected");
//}

//TEST_F(Hdf5DataFrameTest, Factor) {
//    std::vector<data_frame::ColumnDetails> columns(1);
//    columns[0].name = "Aaron";
//    columns[0].type = data_frame::ColumnType::FACTOR;
//    columns[0].factor_levels = std::vector<std::string>{ "kanon", "chisato", "sumire", "ren", "keke" };
//
//    {
//        auto handle = initialize();
//        auto ghandle = handle.createGroup(name);
//        mock(ghandle, 99, columns);
//    }
//    test_validate(dir);
//
//    {
//        auto handle = reopen();
//        auto ghandle = handle.openGroup(name);
//        auto dhandle = ghandle.openGroup("data");
//        auto fhandle = dhandle.openGroup("0");
//        fhandle.unlink("codes");
//        hdf5_utils::spawn_data(fhandle, "codes", 80, H5::PredType::NATIVE_UINT8);
//    }
//    expect_error("length equal to the number of rows");
//
//    {
//        auto handle = reopen();
//        auto ghandle = handle.openGroup(name);
//        auto dhandle = ghandle.openGroup("data");
//        auto fhandle = dhandle.openGroup("0");
//        fhandle.unlink("codes");
//
//        std::vector<int> replacement(99, columns[0].factor_levels.size());
//        auto xhandle = hdf5_utils::spawn_data(fhandle, "codes", replacement.size(), H5::PredType::NATIVE_UINT16);
//        xhandle.write(replacement.data(), H5::PredType::NATIVE_INT);
//    }
//    expect_error("less than the number of levels");
//
//    {
//        auto handle = initialize();
//        auto ghandle = handle.createGroup(name);
//        mock(ghandle, 99, columns);
//
//        auto dhandle = ghandle.openGroup("data");
//        auto fhandle = dhandle.openGroup("0");
//        fhandle.unlink("levels");
//
//        std::vector<std::string> levels(columns[0].factor_levels.begin(), columns[0].factor_levels.end());
//        levels.push_back(levels[0]);
//        hdf5_utils::spawn_string_data(fhandle, "levels", H5T_VARIABLE, levels);
//    }
//    expect_error("duplicated factor level");
//
//    {
//        auto handle = initialize();
//        auto ghandle = handle.createGroup(name);
//        mock(ghandle, 99, columns);
//        auto fhandle = ghandle.openGroup("data/0");
//        fhandle.createAttribute("ordered", H5::PredType::NATIVE_FLOAT, H5S_SCALAR);
//    }
//    expect_error("32-bit signed integer");
//
//    {
//        auto handle = reopen();
//        auto ghandle = handle.openGroup(name);
//        auto fhandle = ghandle.openGroup("data/0");
//        fhandle.removeAttr("ordered");
//        fhandle.createAttribute("ordered", H5::PredType::NATIVE_UINT8, H5S_SCALAR);
//    }
//    test_validate(dir);
//}
//
//TEST_F(Hdf5DataFrameTest, Metadata) {
//    std::vector<data_frame::ColumnDetails> columns(1);
//    columns[0].name = "Aaron";
//    columns[0].type = data_frame::ColumnType::FACTOR;
//    columns[0].factor_levels = std::vector<std::string>{ "kanon", "chisato", "sumire", "ren", "keke" };
//
//    auto cdir = dir / "column_annotations";
//    auto odir = dir / "other_annotations";
//
//    data_frame::mock(dir, 99, columns);
//    initialize_directory_simple(cdir, "simple_list", "1.0");
//    expect_error("'DATA_FRAME'"); 
//
//    data_frame::mock(cdir, columns.size(), {});
//    initialize_directory_simple(odir, "data_frame", "1.0");
//    expect_error("'SIMPLE_LIST'");
//
//    simple_list::mock(odir);
//    test_validate(dir);
//}
//
//TEST_F(Hdf5DataFrameTest, Vls) {
//    std::string heap = "abcdefghijklmno";
//    hsize_t nrows = 10;
//
//    {
//        initialize_directory_simple(dir, "data_frame", "1.1");
//        auto path = dir / "basic_columns.h5";
//        H5::H5File handle(std::string(path), H5F_ACC_TRUNC);
//        auto ghandle = handle.createGroup(name);
//        std::vector<data_frame::ColumnDetails> columns(1);
//        columns[0].name = "superstring";
//        data_frame::mock(ghandle, nrows, columns);
//
//        auto xhandle = ghandle.openGroup("data");
//        xhandle.unlink("0");
//        auto vhandle = xhandle.createGroup("0");
//        hdf5_utils::attach_attribute(vhandle, "type", "vls");
//
//        const unsigned char* hptr = reinterpret_cast<const unsigned char*>(heap.c_str());
//        hsize_t hlen = heap.size();
//        H5::DataSpace hspace(1, &hlen);
//        auto hhandle = vhandle.createDataSet("heap", H5::PredType::NATIVE_UINT8, hspace);
//        hhandle.write(hptr, H5::PredType::NATIVE_UCHAR);
//
//        std::vector<ritsuko::hdf5::vls::Pointer<uint64_t, uint64_t> > pointers(nrows);
//        for (size_t i = 0; i < nrows; ++i) {
//            pointers[i].offset = i; 
//            pointers[i].length = 1;
//        }
//        H5::DataSpace pspace(1, &nrows);
//        auto ptype = ritsuko::hdf5::vls::define_pointer_datatype<uint64_t, uint64_t>();
//        auto phandle = vhandle.createDataSet("pointers", ptype, pspace);
//        phandle.write(pointers.data(), ptype);
//    }
//
//    test_validate(dir);
//
//    // Adding a missing value placeholder.
//    {
//        {
//            auto handle = reopen();
//            auto dhandle = handle.openDataSet("data_frame/data/0/pointers");
//            dhandle.createAttribute("missing-value-placeholder", H5::StrType(0, 10), H5S_SCALAR);
//        }
//        test_validate(dir);
//
//        // Adding the wrong missing value placeholder.
//        {
//            auto handle = reopen();
//            auto dhandle = handle.openDataSet("data_frame/data/0/pointers");
//            dhandle.removeAttr("missing-value-placeholder");
//            dhandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_INT, H5S_SCALAR);
//        }
//        expect_error("string datatype");
//
//        // Removing for the next checks.
//        {
//            auto handle = reopen();
//            auto dhandle = handle.openDataSet("data_frame/data/0/pointers");
//            dhandle.removeAttr("missing-value-placeholder");
//        }
//    }
//
//    // Checking that mismatches in the number of rows is detected.
//    {
//        {
//            auto handle = reopen();
//            auto dhandle = handle.openDataSet("data_frame/data/0/pointers");
//            dhandle.createAttribute("missing-value-placeholder", H5::StrType(0, 10), H5S_SCALAR);
//        }
//        test_validate(dir);
//    }
//
//    // Checking that this only works in the latest version.
//    {
//        auto opath = dir/"OBJECT";
//        auto parsed = millijson::parse_file(opath.c_str(), {});
//        auto& entries = reinterpret_cast<millijson::Object*>(parsed.get())->value();
//        auto& df_entries = reinterpret_cast<millijson::Object*>(entries["data_frame"].get())->value();
//        reinterpret_cast<millijson::String*>(df_entries["version"].get())->value() = "1.0";
//        json_utils::dump(parsed.get(), opath);
//
//        expect_error("unsupported type");
//
//        reinterpret_cast<millijson::String*>(df_entries["version"].get())->value() = "1.1";
//        json_utils::dump(parsed.get(), opath);
//    }
//
//    // Shortening the heap to check that we perform bounds checks on the pointers.
//    {
//        {
//            auto handle = reopen();
//            auto vhandle = handle.openGroup("data_frame/data/0");
//            vhandle.unlink("heap");
//            hsize_t zero = 0;
//            H5::DataSpace hspace(1, &zero);
//            vhandle.createDataSet("heap", H5::PredType::NATIVE_UINT8, hspace);
//        }
//        expect_error("out of range");
//    }
//
//    // Checking that we check for 64-bit unsigned integer types. 
//    {
//        {
//            auto handle = reopen();
//            auto vhandle = handle.openGroup("data_frame/data/0");
//            vhandle.unlink("pointers");
//
//            std::vector<ritsuko::hdf5::vls::Pointer<int, int> > pointers(nrows);
//            for (auto& pp : pointers) {
//                pp.offset = 0;
//                pp.length = 0;
//            }
//            H5::DataSpace pspace(1, &nrows);
//            auto ptype = ritsuko::hdf5::vls::define_pointer_datatype<int, int>();
//            auto phandle = vhandle.createDataSet("pointers", ptype, pspace);
//            phandle.write(pointers.data(), ptype);
//        }
//        expect_error("64-bit unsigned integer");
//    }
//
//    // Checking that we check for 64-bit unsigned integer types. 
//    {
//        {
//            auto handle = reopen();
//            auto vhandle = handle.openGroup("data_frame/data/0");
//            vhandle.unlink("pointers");
//
//            hsize_t nrows_p1 = nrows + 1;
//            std::vector<ritsuko::hdf5::vls::Pointer<uint8_t, uint8_t> > pointers(nrows_p1);
//            for (auto& pp : pointers) {
//                pp.offset = 0;
//                pp.length = 0;
//            }
//            H5::DataSpace pspace(1, &nrows_p1);
//            auto ptype = ritsuko::hdf5::vls::define_pointer_datatype<uint8_t, uint8_t>();
//            auto phandle = vhandle.createDataSet("pointers", ptype, pspace);
//            phandle.write(pointers.data(), ptype);
//        }
//        expect_error("number of rows");
//    }
//}
