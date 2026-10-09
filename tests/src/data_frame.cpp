#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "mock_data_frame.h"
#include "mock_atomic_vector.h"
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
        ghandle.createDataSet("row_names", H5::PredType::NATIVE_INT32, create_hdf5_dataspace(29));
    }
    expect_validation_error(dir, "represented by a UTF-8 encoded string");

    {
        auto ghandle = mock_data_frame(dir, 29, columns);
        ghandle.createDataSet("row_names", H5::StrType(0, 10), H5S_SCALAR);
    }
    expect_validation_error(dir, "1-dimensional");

    {
        auto ghandle = mock_data_frame(dir, 29, columns);
        ghandle.createDataSet("row_names", H5::StrType(0, 10), create_hdf5_dataspace(30));
    }
    expect_validation_error(dir, "number of row names");

    {
        auto ghandle = mock_data_frame(dir, 29, columns);
        ghandle.createDataSet("row_names", H5::StrType(0, H5T_VARIABLE), create_hdf5_dataspace(29));
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
        ghandle.createDataSet("column_names", H5::PredType::NATIVE_INT, create_hdf5_dataspace(columns.size()));
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
}

TEST(DataFrame, ColumnDatasetError) {
    auto dir = define_test_path("data_frame");

    std::vector<DataFrameColumnDetails> columns(2);
    columns[0].name = "Aaron";
    columns[1].name = "Barry";
    columns[1].type = DataFrameColumnType::STRING;

    {
        auto ghandle = mock_data_frame(dir, 33, columns);
        auto dhandle = ghandle.openGroup("data");
        auto xhandle = dhandle.openDataSet("1");
        xhandle.removeAttr("type");
        add_hdf5_string_attribute(xhandle, "type", "something");
    }
    expect_validation_error(dir, "unknown column type");

    {
        auto ghandle = mock_data_frame(dir, 33, columns);
        auto dhandle = ghandle.openGroup("data");
        dhandle.unlink("0");
        dhandle.createDataSet("0", H5::PredType::NATIVE_INT32, H5S_SCALAR);
    }
    expect_validation_error(dir, "1-dimensional");

    {
        auto ghandle = mock_data_frame(dir, 33, columns);
        auto dhandle = ghandle.openGroup("data");
        dhandle.unlink("0");
        auto xhandle = dhandle.createDataSet("0", H5::PredType::NATIVE_INT32, create_hdf5_dataspace(32));
        add_hdf5_string_attribute(xhandle, "type", "integer");
    }
    expect_validation_error(dir, "not equal to the number of rows");
}

TEST(DataFrame, ColumnGroupError) {
    auto dir = define_test_path("data_frame");

    std::vector<DataFrameColumnDetails> columns(1);
    columns[0].name = "Delta";
    columns[0].type = DataFrameColumnType::FACTOR;
    columns[0].factor_levels = std::vector<std::string>{ "chihaya", "haruka", "miki" };

    {
        auto ghandle = mock_data_frame(dir, 33, columns);
        auto dhandle = ghandle.openGroup("data");
        auto xhandle = dhandle.openGroup("0");
        xhandle.removeAttr("type");
        add_hdf5_string_attribute(xhandle, "type", "something");
    }
    expect_validation_error(dir, "unknown column type");
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
        add_hdf5_string_attribute(xhandle, "type", "boolean");
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
        add_hdf5_string_attribute(xhandle, "type", "integer");
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
        auto xhandle = dhandle.createDataSet("0", H5::PredType::NATIVE_INT64, create_hdf5_dataspace(55));
        add_hdf5_string_attribute(xhandle, "type", "number");
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
        add_hdf5_string_attribute(xhandle, "format", "none");
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
        add_hdf5_string_attribute(xhandle, "type", "string");
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
        add_hdf5_string_attribute(xhandle, "format", "foobar"); 
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
        add_hdf5_string_attribute(xhandle, "missing-value-placeholder", "asdasd");
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
        add_hdf5_string_attribute(xhandle, "format", "date");
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
        add_hdf5_string_attribute(xhandle, "format", "date");
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
        add_hdf5_string_attribute(xhandle, "format", "date");
        const char* placeholder = "2026-09-29";
        const char* dummy = "mitochondria";
        std::vector<const char*> pointers(len, placeholder);
        pointers[len / 2] = dummy;
        xhandle.write(pointers.data(), H5::StrType(0, H5T_VARIABLE));
        add_hdf5_string_attribute(xhandle, "missing-value-placeholder", dummy);
    }

    test_validate(dir);
    EXPECT_EQ(test_height(dir), len);
    std::vector<std::size_t> expected_dim;
    expected_dim.push_back(len);
    expected_dim.push_back(1);
    EXPECT_EQ(test_dimensions(dir), expected_dim);
}

/**************************************/

TEST(DataFrame, FactorOkay) {
    auto dir = define_test_path("data_frame");

    std::vector<DataFrameColumnDetails> columns(1);
    columns[0].name = "Aaron";
    columns[0].type = DataFrameColumnType::FACTOR;
    columns[0].factor_levels = std::vector<std::string>{ "kanon", "chisato", "sumire", "ren", "keke" };

    {
        mock_data_frame(dir, 99, columns);
    }
    {
        test_validate(dir);
        EXPECT_EQ(test_height(dir), 99);
        std::vector<std::size_t> expected_dim{ 99, 1 };
        EXPECT_EQ(test_dimensions(dir), expected_dim);
    }

    // Plus the ordered attribute.
    {
        auto ghandle = mock_data_frame(dir, 99, columns);
        auto dhandle = ghandle.openGroup("data");
        auto xhandle = dhandle.openGroup("0");
        xhandle.createAttribute("ordered", H5::PredType::NATIVE_UINT8, H5S_SCALAR);
    }
    {
        test_validate(dir);
        EXPECT_EQ(test_height(dir), 99);
        std::vector<std::size_t> expected_dim{ 99, 1 };
        EXPECT_EQ(test_dimensions(dir), expected_dim);
    }
}

TEST(DataFrame, FactorError) {
    auto dir = define_test_path("data_frame");

    std::vector<DataFrameColumnDetails> columns(1);
    columns[0].name = "Aaron";
    columns[0].type = DataFrameColumnType::FACTOR;
    columns[0].factor_levels = std::vector<std::string>{ "kanon", "chisato", "sumire", "ren", "keke" };

    // Check that the ordered attribute is checked.
    {
        auto ghandle = mock_data_frame(dir, 99, columns);
        auto dhandle = ghandle.openGroup("data");
        auto fhandle = dhandle.openGroup("0");
        fhandle.createAttribute("ordered", H5::PredType::NATIVE_DOUBLE, H5S_SCALAR);
    }
    expect_validation_error(dir, "32-bit signed integer");

    // Check that levels are correctly validated.
    {
        auto ghandle = mock_data_frame(dir, 99, columns);
        auto dhandle = ghandle.openGroup("data");
        auto fhandle = dhandle.openGroup("0");
        fhandle.unlink("levels");
        fhandle.createDataSet("levels", H5::StrType(0, 10), create_hdf5_dataspace(5));
    }
    expect_validation_error(dir, "duplicated factor level");

    // Check that codes are correctly validated.
    {
        auto ghandle = mock_data_frame(dir, 99, columns);
        auto dhandle = ghandle.openGroup("data");
        auto fhandle = dhandle.openGroup("0");
        fhandle.unlink("levels");
        fhandle.createDataSet("levels", H5::StrType(0, 10), create_hdf5_dataspace(1));
    }
    expect_validation_error(dir, "less than the number of levels");

    // Check that we correctly pass along the length to outer checks. 
    {
        auto ghandle = mock_data_frame(dir, 99, columns);
        auto dhandle = ghandle.openGroup("data");
        auto fhandle = dhandle.openGroup("0");
        fhandle.unlink("codes");
        auto xhandle = fhandle.createDataSet("codes", H5::PredType::NATIVE_UINT8, create_hdf5_dataspace(80));
        std::vector<int> data(80);
        xhandle.write(data.data(), H5::PredType::NATIVE_INT);
    }
    expect_validation_error(dir, "not equal to the number of rows");
}

/**************************************/

TEST(DataFrame, VlsVersionError) {
    auto dir = define_test_path("data_frame");

    std::vector<DataFrameColumnDetails> columns(1);
    columns[0].name = "Aaron";
    columns[0].type = DataFrameColumnType::VLS;

    {
        mock_data_frame(dir, 99, columns);
        std::ofstream out(dir / "OBJECT");
        out << "{ \"type\":\"data_frame\", \"data_frame\":{ \"version\": \"1.0\" } }";
    }
    expect_validation_error(dir, "unsupported type");
}

TEST(DataFrame, VlsHeapError) {
    auto dir = define_test_path("data_frame");

    std::vector<DataFrameColumnDetails> columns(1);
    columns[0].name = "Aaron";
    columns[0].type = DataFrameColumnType::VLS;

    // Check that we validate the heap.
    {
        auto ghandle = mock_data_frame(dir, 99, columns);
        auto dhandle = ghandle.openGroup("data");
        auto xhandle = dhandle.openGroup("0");
        xhandle.unlink("heap");
        xhandle.createDataSet("heap", H5::PredType::NATIVE_INT32, create_hdf5_dataspace(100));
    }
    expect_validation_error(dir, "8-bit unsigned integer");
}

TEST(DataFrame, VlsPointersError) {
    auto dir = define_test_path("data_frame");

    std::vector<DataFrameColumnDetails> columns(1);
    columns[0].name = "Aaron";
    columns[0].type = DataFrameColumnType::VLS;

    {
        auto ghandle = mock_data_frame(dir, 99, columns);
        auto dhandle = ghandle.openGroup("data");
        auto xhandle = dhandle.openGroup("0");
        xhandle.unlink("pointers");
        xhandle.createDataSet("pointers", ritsuko::cvls::define_pointer_datatype<std::uint64_t, std::uint64_t>(), H5S_SCALAR);
    }
    expect_validation_error(dir, "1-dimensional");

    // Check that we validate the pointers.
    {
        const hsize_t num_rows = 99;
        auto ghandle = mock_data_frame(dir, num_rows, columns);
        auto dhandle = ghandle.openGroup("data");
        auto xhandle = dhandle.openGroup("0");
        auto phandle = xhandle.openDataSet("pointers");

        std::vector<ritsuko::cvls::Pointer<std::uint64_t, std::uint64_t> > buffer(num_rows);
        for (hsize_t i = 0; i < num_rows; ++i) {
            buffer[i].offset = 0; 
            buffer[i].length = -1;
        }
        phandle.write(buffer.data(), ritsuko::cvls::define_pointer_datatype<std::uint64_t, std::uint64_t>());
    }
    expect_validation_error(dir, "out of range of the heap");

    // Check that the length is properly passed along.
    {
        auto ghandle = mock_data_frame(dir, 99, columns);
        auto dhandle = ghandle.openGroup("data");
        auto xhandle = dhandle.openGroup("0");
        xhandle.unlink("pointers");

        const hsize_t new_num_rows = 100;
        auto phandle = xhandle.createDataSet("pointers", ritsuko::cvls::define_pointer_datatype<std::uint64_t, std::uint64_t>(), create_hdf5_dataspace(new_num_rows));
        std::vector<ritsuko::cvls::Pointer<std::uint64_t, std::uint64_t> > buffer(new_num_rows);
        for (hsize_t i = 0; i < new_num_rows; ++i) {
            buffer[i].offset = 0; 
            buffer[i].length = 0;
        }
        phandle.write(buffer.data(), ritsuko::cvls::define_pointer_datatype<std::uint64_t, std::uint64_t>());
    }
    expect_validation_error(dir, "not equal to the number of rows");
}

TEST(DataFrame, VlsMissingOkay) {
    auto dir = define_test_path("data_frame");

    std::vector<DataFrameColumnDetails> columns(1);
    columns[0].name = "Aaron";
    columns[0].type = DataFrameColumnType::VLS;

    {
        auto ghandle = mock_data_frame(dir, 82, columns);
        auto dhandle = ghandle.openGroup("data");
        auto xhandle = dhandle.openGroup("0");
        auto phandle = xhandle.openDataSet("pointers");
        add_hdf5_string_attribute(phandle, "missing-value-placeholder", "foobar");
    }

    test_validate(dir);
    EXPECT_EQ(test_height(dir), 82);
    std::vector<std::size_t> expected_dim{ 82, 1 };
    EXPECT_EQ(test_dimensions(dir), expected_dim);
}

TEST(DataFrame, VlsMissingError) {
    auto dir = define_test_path("data_frame");

    std::vector<DataFrameColumnDetails> columns(1);
    columns[0].name = "Aaron";
    columns[0].type = DataFrameColumnType::VLS;

    {
        auto ghandle = mock_data_frame(dir, 82, columns);
        auto dhandle = ghandle.openGroup("data");
        auto xhandle = dhandle.openGroup("0");
        auto phandle = xhandle.openDataSet("pointers");
        phandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_INT, H5S_SCALAR);
    }
    expect_validation_error(dir, "UTF-8 string");
}

/**************************************/

TEST(DataFrame, OtherOkay) {
    auto dir = define_test_path("data_frame");

    std::vector<DataFrameColumnDetails> columns(3);
    columns[0].name = "Kanon";
    columns[0].type = DataFrameColumnType::OTHER;
    columns[1].name = "Chisato";
    columns[1].type = DataFrameColumnType::STRING; // intervening basic column, for some variety.
    columns[2].name = "Keke";
    columns[2].type = DataFrameColumnType::OTHER;

    {
        mock_data_frame(dir, 51, columns);
        initialize_directory(dir / "other_columns");

        std::vector<DataFrameColumnDetails> subcolumns(1);
        subcolumns[0].name = "shibuya"; 
        mock_data_frame(dir / "other_columns" / "0", 51, subcolumns);

        mock_atomic_vector(dir / "other_columns" / "2", 51, AtomicVectorType::STRING);
    }

    test_validate(dir);
    EXPECT_EQ(test_height(dir), 51);
    std::vector<std::size_t> expected{ 51, 3 };
    EXPECT_EQ(test_dimensions(dir), expected);
}

TEST(DataFrame, OtherError) {
    auto dir = define_test_path("data_frame");

    std::vector<DataFrameColumnDetails> columns(2);
    columns[0].name = "Sumire";
    columns[0].type = DataFrameColumnType::OTHER;
    columns[1].name = "Ren";
    columns[1].type = DataFrameColumnType::OTHER;

    {
        mock_data_frame(dir, 51, columns);
        initialize_directory(dir / "other_columns");
        std::vector<DataFrameColumnDetails> subcolumns(1);
        subcolumns[0].name = "heanna";
        mock_data_frame(dir / "other_columns" / "0", 32, subcolumns);
        mock_atomic_vector(dir / "other_columns" / "1", 51, AtomicVectorType::STRING);
    }
    expect_validation_error(dir, "height is not equal");

    {
        mock_data_frame(dir, 51, columns);
        initialize_directory(dir / "other_columns");
        initialize_directory_simple(dir / "other_columns" / "0", "superfoobar", "1.0");
        mock_atomic_vector(dir / "other_columns" / "1", 51, AtomicVectorType::STRING);
    }
    expect_validation_error(dir, "failed to validate column 0");

    {
        mock_data_frame(dir, 51, columns);
        initialize_directory(dir / "other_columns");
        mock_atomic_vector(dir / "other_columns" / "0", 51, AtomicVectorType::INTEGER);
        mock_atomic_vector(dir / "other_columns" / "1", 51, AtomicVectorType::STRING);
        mock_atomic_vector(dir / "other_columns" / "2", 51, AtomicVectorType::STRING);
    }
    expect_validation_error(dir, "more objects than expected");
}

/**************************************/

TEST(DataFrame, McolsOkay) {
    auto dir = define_test_path("data_frame");

    std::vector<DataFrameColumnDetails> columns(2);
    columns[0].name = "nagisa";
    columns[1].name = "tomoya";

    std::vector<DataFrameColumnDetails> metacolumns(3);
    metacolumns[0].name = "akio";
    metacolumns[0].type = DataFrameColumnType::NUMBER;
    metacolumns[1].name = "sanae";
    metacolumns[1].type = DataFrameColumnType::STRING;
    metacolumns[2].name = "ushio";
    metacolumns[2].type = DataFrameColumnType::BOOLEAN;

    {
        mock_data_frame(dir, 99, columns);
        mock_data_frame(dir / "column_annotations", 2, metacolumns);
    }

    test_validate(dir);
}

TEST(DataFrame, McolsError) {
    auto dir = define_test_path("data_frame");

    std::vector<DataFrameColumnDetails> columns(2);
    columns[0].name = "furukawa";
    columns[1].name = "okazaki";

    // Check that we actually validate the mcols.
    {
        mock_data_frame(dir, 99, columns);
        mock_data_frame(dir / "column_annotations", 4, {});
    }
    expect_validation_error(dir, "unexpected number of rows");
}

TEST(DataFrame, MetadataOkay) {
    auto dir = define_test_path("data_frame");

    std::vector<DataFrameColumnDetails> columns(4);
    columns[0].name = "dango daikazoku";
    columns[0].type = DataFrameColumnType::NUMBER;
    columns[1].name = "toki wo izuma uta";
    columns[1].type = DataFrameColumnType::STRING;
    columns[2].name = "the palm of a tiny hand";
    columns[2].type = DataFrameColumnType::BOOLEAN;
    columns[3].name = "over";

    {
        mock_data_frame(dir, 13, columns);
        mock_simple_list(dir / "other_annotations");
    }

    test_validate(dir);
}

TEST(DataFrame, MetadataError) {
    auto dir = define_test_path("data_frame");

    std::vector<DataFrameColumnDetails> columns(2);
    columns[0].name = "cherry blossom petal";
    columns[1].name = "roaring ocean";

    // Check that we actually validate the metadata.
    {
        mock_data_frame(dir, 13, columns);
        mock_data_frame(dir / "other_annotations", 2, {});
    }
    expect_validation_error(dir, "SIMPLE_LIST");
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

    {
        std::vector<DataFrameColumnDetails> columns(1);
        columns[0].name = "Aaron";
        auto ghandle = mock_data_frame(dir, 33, columns);
        auto dhandle = ghandle.openGroup("data");
        dhandle.createGroup("foo");
    }
    expect_validation_error(dir, "more objects present");
}
