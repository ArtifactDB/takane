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

static DataFrameColumn create_factor_column(std::string name, std::vector<std::string> levels, bool ordered) {
    DataFrameColumn output(name, DataFrameColumnType::FACTOR);
    output.factor_levels = std::move(levels);
    output.factor_ordered = ordered;
    return output;
}

TEST(DataFrame, MixedOkay) {
    auto dir = define_test_path("data_frame");

    // DataFrame with many different column types works correctly.
    DataFrameOptions opt(51, {
        DataFrameColumn("Aaron", DataFrameColumnType::BOOLEAN),
        DataFrameColumn("Barry", DataFrameColumnType::NUMBER),
        DataFrameColumn("Charlie", DataFrameColumnType::STRING),
        DataFrameColumn("Delta", DataFrameColumnType::INTEGER),
        create_factor_column("Echo", { "akari", "aika", "alice", "ai" }, false),
        create_factor_column("Foxtrot", { "athena", "akira", "alicia" }, true)
    });

    {
        mock_data_frame(dir, opt);
    }
    {
        test_validate(dir);
        EXPECT_EQ(test_height(dir), 51);
        EXPECT_EQ(test_dimensions(dir), (std::vector<std::size_t>{ 51, 6 }));
    }

    // Injecting a VLS column.
    opt.columns.emplace_back("Gamma", DataFrameColumnType::VLS);
    opt.version = "1.1";
    {
        mock_data_frame(dir, opt);
    }
    {
        test_validate(dir);
        EXPECT_EQ(test_height(dir), 51);
        EXPECT_EQ(test_dimensions(dir), (std::vector<std::size_t>{ 51, 7 }));
    }
}

TEST(DataFrame, EmptyOkay) {
    auto dir = define_test_path("data_frame");

    {
        mock_data_frame(dir, DataFrameOptions(11, {}));
    }
    {
        test_validate(dir);
        EXPECT_EQ(test_height(dir), 11);
        EXPECT_EQ(test_dimensions(dir), (std::vector<std::size_t>{ 11, 0 }));
    }

    {
        mock_data_frame(dir, DataFrameOptions(0, {
            DataFrameColumn("foo"),
            create_factor_column("bar", {}, false)
        }));
    }
    {
        test_validate(dir);
        EXPECT_EQ(test_height(dir), 0);
        EXPECT_EQ(test_dimensions(dir), (std::vector<std::size_t>{ 0, 2 }));
    }
}

/**************************************/

TEST(DataFrame, RownamesOkay) {
    auto dir = define_test_path("data_frame");

    {
        DataFrameOptions opt(29, { DataFrameColumn("WHEE") });
        auto ghandle = mock_data_frame(dir, opt);
        std::vector<std::string> row_names(opt.num_rows);
        for (hsize_t i = 0; i < opt.num_rows; ++i) {
            row_names[i] = std::to_string(i);
        }
        add_hdf5_string_dataset(ghandle, "row_names", row_names);
    }

    test_validate(dir);
    EXPECT_EQ(test_height(dir), 29);
    EXPECT_EQ(test_dimensions(dir), (std::vector<std::size_t>{ 29, 1 }));
}

TEST(DataFrame, RownamesError) {
    auto dir = define_test_path("data_frame");

    DataFrameOptions opt(29, { DataFrameColumn("WHEE") });

    {
        auto ghandle = mock_data_frame(dir, opt); 
        ghandle.createDataSet("row_names", H5::PredType::NATIVE_INT32, create_hdf5_dataspace(29));
    }
    expect_validation_error(dir, "represented by a UTF-8 encoded string");

    {
        auto ghandle = mock_data_frame(dir, opt);
        ghandle.createDataSet("row_names", H5::StrType(0, 10), H5S_SCALAR);
    }
    expect_validation_error(dir, "1-dimensional");

    {
        auto ghandle = mock_data_frame(dir, opt);
        ghandle.createDataSet("row_names", H5::StrType(0, 10), create_hdf5_dataspace(30));
    }
    expect_validation_error(dir, "number of row names");

    {
        auto ghandle = mock_data_frame(dir, opt);
        ghandle.createDataSet("row_names", H5::StrType(0, H5T_VARIABLE), create_hdf5_dataspace(29));
    }
    expect_validation_error(dir, "failed to validate");
}

/**************************************/

TEST(DataFrame, ColnamesOkay) {
    auto dir = define_test_path("data_frame");

    {
        DataFrameOptions opt(29, {
            DataFrameColumn("Aaron", DataFrameColumnType::NUMBER),
            DataFrameColumn("Barry", DataFrameColumnType::STRING)
        });
        mock_data_frame(dir, opt);
    }

    test_validate(dir);
    EXPECT_EQ(test_height(dir), 29);
    std::vector<size_t> expected_dim{ 29, 2 };
    EXPECT_EQ(test_dimensions(dir), expected_dim);
}

TEST(DataFrame, ColnamesError) {
    auto dir = define_test_path("data_frame");

    DataFrameOptions opt(29, {
        DataFrameColumn("Aaron", DataFrameColumnType::NUMBER),
        DataFrameColumn("Barry", DataFrameColumnType::BOOLEAN),
        DataFrameColumn("Charlie")
    });

    {
        auto ghandle = mock_data_frame(dir, opt);
        ghandle.unlink("column_names");
        ghandle.createDataSet("column_names", H5::PredType::NATIVE_INT, create_hdf5_dataspace(3));
    }
    expect_validation_error(dir, "UTF-8 encoded string");

    {
        auto ghandle = mock_data_frame(dir, opt);
        ghandle.unlink("column_names");
        ghandle.createDataSet("column_names", H5::StrType(0, 5), H5S_SCALAR);
    }
    expect_validation_error(dir, "1-dimensional");

    {
        auto optcopy = opt;
        optcopy.columns[1].name = "Aaron";
        mock_data_frame(dir, optcopy);
    }
    expect_validation_error(dir, "duplicated column name");

    {
        auto optcopy = opt;
        optcopy.columns[0].name = "";
        mock_data_frame(dir, optcopy);
    }
    expect_validation_error(dir, "empty strings");
}

/**************************************/


TEST(DataFrame, ColumnDatasetError) {
    auto dir = define_test_path("data_frame");

    DataFrameOptions opt(33, {
        DataFrameColumn("Aaron"),
        DataFrameColumn("Barry", DataFrameColumnType::STRING)
    });

    {
        auto ghandle = mock_data_frame(dir, opt);
        auto dhandle = ghandle.openGroup("data");
        auto xhandle = dhandle.openDataSet("1");
        xhandle.removeAttr("type");
        add_hdf5_string_attribute(xhandle, "type", "something");
    }
    expect_validation_error(dir, "unknown column type");

    {
        auto ghandle = mock_data_frame(dir, opt);
        auto dhandle = ghandle.openGroup("data");
        dhandle.unlink("0");
        dhandle.createDataSet("0", H5::PredType::NATIVE_INT32, H5S_SCALAR);
    }
    expect_validation_error(dir, "1-dimensional");

    {
        auto ghandle = mock_data_frame(dir, opt);
        auto dhandle = ghandle.openGroup("data");
        dhandle.unlink("0");
        auto xhandle = dhandle.createDataSet("0", H5::PredType::NATIVE_INT32, create_hdf5_dataspace(32));
        add_hdf5_string_attribute(xhandle, "type", "integer");
    }
    expect_validation_error(dir, "not equal to the number of rows");
}

TEST(DataFrame, ColumnGroupError) {
    auto dir = define_test_path("data_frame");

    {
        DataFrameOptions opt(33, { create_factor_column("Delta", { "chihaya", "haruka", "miki" }, false) });
        auto ghandle = mock_data_frame(dir, opt);
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

    {
        DataFrameOptions opt(54, { DataFrameColumn("Kyoko", DataFrameColumnType::NUMBER) });
        auto ghandle = mock_data_frame(dir, opt);
        auto dhandle = ghandle.openGroup("data");
        auto xhandle = dhandle.openDataSet("0");
        xhandle.removeAttr("type");
        add_hdf5_string_attribute(xhandle, "type", "boolean");
    }
    expect_validation_error(dir, "32-bit signed integer");
}

TEST(DataFrame, IntegerError) {
    auto dir = define_test_path("data_frame");

    {
        DataFrameOptions opt(48, { DataFrameColumn("Kyubey", DataFrameColumnType::NUMBER) });
        auto ghandle = mock_data_frame(dir, opt);
        auto dhandle = ghandle.openGroup("data");
        auto xhandle = dhandle.openDataSet("0");
        xhandle.removeAttr("type");
        add_hdf5_string_attribute(xhandle, "type", "integer");
    }
    expect_validation_error(dir, "32-bit signed integer");
}

TEST(DataFrame, NumberError) {
    auto dir = define_test_path("data_frame");

    {
        DataFrameOptions opt(49, { DataFrameColumn("Madoka", DataFrameColumnType::STRING) });
        auto ghandle = mock_data_frame(dir, opt);
        auto dhandle = ghandle.openGroup("data");
        auto xhandle = dhandle.openDataSet("0");
        xhandle.removeAttr("type");
        add_hdf5_string_attribute(xhandle, "type", "number");
    }
    expect_validation_error(dir, "64-bit float");
}

TEST(DataFrame, NumericMissingOkay) {
    auto dir = define_test_path("data_frame");

    {
        DataFrameOptions opt(55, {
            DataFrameColumn("Madoka", DataFrameColumnType::BOOLEAN),
            DataFrameColumn("Homura", DataFrameColumnType::INTEGER),
            DataFrameColumn("Sayaka", DataFrameColumnType::NUMBER)
        });
        auto ghandle = mock_data_frame(dir, opt);
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
    EXPECT_EQ(test_dimensions(dir), (std::vector<std::size_t>{ 55, 3 }));
}

TEST(DataFrame, NumericMissingError) {
    auto dir = define_test_path("data_frame");

    for (int i = 0; i < 3; ++i) {
        DataFrameOptions opt(21, { DataFrameColumn("Mami") });
        if (i == 0) {
            opt.columns[0].type = DataFrameColumnType::INTEGER;
        } else if (i == 1) {
            opt.columns[0].type = DataFrameColumnType::BOOLEAN;
        } else {
            opt.columns[0].type = DataFrameColumnType::NUMBER;
        }

        {
            auto ghandle = mock_data_frame(dir, opt);
            auto dhandle = ghandle.openGroup("data");
            auto xhandle = dhandle.openDataSet("0");
            xhandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_INT16, H5S_SCALAR);
        }
        expect_validation_error(dir, "same datatype");
    }
}

/**************************************/

TEST(DataFrame, StringError) {
    auto dir = define_test_path("data_frame");

    {
        auto ghandle = mock_data_frame(dir, DataFrameOptions(23, { DataFrameColumn("Kongou") })); 
        auto dhandle = ghandle.openGroup("data");
        auto xhandle = dhandle.openDataSet("0");
        xhandle.removeAttr("type");
        add_hdf5_string_attribute(xhandle, "type", "string");
    }
    expect_validation_error(dir, "represented by a UTF-8 encoded string");

    // Check that we validate the format.
    {
        auto ghandle = mock_data_frame(dir, DataFrameOptions(23, { DataFrameColumn("Kongou", DataFrameColumnType::STRING) })); 
        auto dhandle = ghandle.openGroup("data");
        auto xhandle = dhandle.openDataSet("0");
        xhandle.createAttribute("format", H5::StrType(0, 10), create_hdf5_dataspace(12));
    }
    expect_validation_error(dir, "scalar");

    {
        auto ghandle = mock_data_frame(dir, DataFrameOptions(23, { DataFrameColumn("Kongou", DataFrameColumnType::STRING) })); 
        auto dhandle = ghandle.openGroup("data");
        auto xhandle = dhandle.openDataSet("0");
        add_hdf5_string_attribute(xhandle, "format", "foobar"); 
    }
    expect_validation_error(dir, "foobar");

    // Confirm that we validate the string contents.
    {
        DataFrameOptions opt(23, { DataFrameColumn("Kongou", DataFrameColumnType::STRING) });
        opt.columns[0].string_length = 0;
        mock_data_frame(dir, opt);
    }
    expect_validation_error(dir, "NULL");
}

TEST(DataFrame, StringNoneOkay) {
    auto dir = define_test_path("data_frame");

    // Injecting a 'none' format to ensure that is respected.
    {
        auto ghandle = mock_data_frame(dir, DataFrameOptions(32, { DataFrameColumn("Haruna", DataFrameColumnType::STRING) }));
        auto dhandle = ghandle.openGroup("data");
        auto xhandle = dhandle.openDataSet("0");
        add_hdf5_string_attribute(xhandle, "format", "none");
    }

    test_validate(dir);
    EXPECT_EQ(test_height(dir), 32);
    EXPECT_EQ(test_dimensions(dir), (std::vector<std::size_t>{ 32, 1 }));
}

TEST(DataFrame, StringMissingOkay) {
    auto dir = define_test_path("data_frame");

    {
        auto ghandle = mock_data_frame(dir, DataFrameOptions(14, { DataFrameColumn("Kirishima", DataFrameColumnType::STRING) })); 
        auto dhandle = ghandle.openGroup("data");
        auto xhandle = dhandle.openDataSet("0");
        add_hdf5_string_attribute(xhandle, "missing-value-placeholder", "asdasd");
    }

    test_validate(dir);
    EXPECT_EQ(test_height(dir), 14);
    EXPECT_EQ(test_dimensions(dir), (std::vector<std::size_t>{ 14, 1 }));
}

TEST(DataFrame, StringMissingError) {
    auto dir = define_test_path("data_frame");

    {
        auto ghandle = mock_data_frame(dir, DataFrameOptions(14, { DataFrameColumn("Hiei", DataFrameColumnType::STRING) })); 
        auto dhandle = ghandle.openGroup("data");
        auto xhandle = dhandle.openDataSet("0");
        xhandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_INT8, H5S_SCALAR);
    }
    expect_validation_error(dir, "UTF-8 string");
}

TEST(DataFrame, StringFormatOkay) {
    auto dir = define_test_path("data_frame");

    {
        DataFrameOptions opt(17, { DataFrameColumn("Akagi", DataFrameColumnType::STRING) });
        opt.columns[0].string_length = 0;
        auto ghandle = mock_data_frame(dir, opt);

        auto dhandle = ghandle.openGroup("data");
        auto xhandle = dhandle.openDataSet("0");
        add_hdf5_string_attribute(xhandle, "format", "date");

        const char* placeholder = "2026-09-29";
        std::vector<const char*> pointers(opt.num_rows, placeholder);
        xhandle.write(pointers.data(), H5::StrType(0, H5T_VARIABLE));
    }

    test_validate(dir);
    EXPECT_EQ(test_height(dir), 17);
    EXPECT_EQ(test_dimensions(dir), (std::vector<std::size_t>{ 17, 1 }));
}

TEST(DataFrame, StringFormatError) {
    auto dir = define_test_path("data_frame");

    {
        DataFrameOptions opt(17, { DataFrameColumn("Kaga", DataFrameColumnType::STRING) });
        opt.columns[0].string_length = 0;
        auto ghandle = mock_data_frame(dir, opt);

        auto dhandle = ghandle.openGroup("data");
        auto xhandle = dhandle.openDataSet("0");
        add_hdf5_string_attribute(xhandle, "format", "date");

        const char* dummy = "mitochondria";
        const char* placeholder = "2026-09-29";
        std::vector<const char*> pointers(opt.num_rows, placeholder);
        pointers[opt.num_rows / 2] = dummy;
        xhandle.write(pointers.data(), H5::StrType(0, H5T_VARIABLE));
    }

    expect_validation_error(dir, "date-formatted string");
}

TEST(DataFrame, StringFormatMissingOkay) {
    auto dir = define_test_path("data_frame");

    // We correctly pass along the missing placeholder to the format checker.
    {
        DataFrameOptions opt(35, { DataFrameColumn("Shoukaku", DataFrameColumnType::STRING) });
        opt.columns[0].string_length = 0;
        auto ghandle = mock_data_frame(dir, opt);

        auto dhandle = ghandle.openGroup("data");
        auto xhandle = dhandle.openDataSet("0");
        add_hdf5_string_attribute(xhandle, "format", "date");

        const char* dummy = "mitochondria";
        add_hdf5_string_attribute(xhandle, "missing-value-placeholder", dummy);

        const char* placeholder = "2026-09-29";
        std::vector<const char*> pointers(opt.num_rows, placeholder);
        pointers[opt.num_rows / 2] = dummy;
        xhandle.write(pointers.data(), H5::StrType(0, H5T_VARIABLE));
    }

    test_validate(dir);
    EXPECT_EQ(test_height(dir), 35);
    EXPECT_EQ(test_dimensions(dir), (std::vector<std::size_t>{ 35, 1 }));
}

/**************************************/

TEST(DataFrame, FactorError) {
    auto dir = define_test_path("data_frame");

    DataFrameOptions opt(99, { create_factor_column("liella", { "kanon", "chisato", "sumire", "ren", "keke" }, false) });

    // Check that the ordered attribute is checked.
    {
        auto ghandle = mock_data_frame(dir, opt);
        auto dhandle = ghandle.openGroup("data");
        auto fhandle = dhandle.openGroup("0");
        fhandle.createAttribute("ordered", H5::PredType::NATIVE_DOUBLE, H5S_SCALAR);
    }
    expect_validation_error(dir, "32-bit signed integer");

    // Check that levels are correctly validated.
    {
        mock_data_frame(dir, DataFrameOptions(99, { create_factor_column("liella", { "shibuya", "heanna", "shibuya" }, false) }));
    }
    expect_validation_error(dir, "duplicated factor level");

    // Check that codes are correctly validated.
    {
        auto ghandle = mock_data_frame(dir, opt);
        auto dhandle = ghandle.openGroup("data");
        auto fhandle = dhandle.openGroup("0");
        fhandle.unlink("levels");
        add_hdf5_string_dataset(fhandle, "levels", { "ren" });
    }
    expect_validation_error(dir, "less than the number of levels");

    // Check that we correctly pass along the length to outer checks. 
    {
        auto ghandle = mock_data_frame(dir, opt);
        auto dhandle = ghandle.openGroup("data");
        auto fhandle = dhandle.openGroup("0");
        fhandle.unlink("codes");
        add_hdf5_numeric_dataset(fhandle, "codes", H5::PredType::NATIVE_UINT8, std::vector<int>(80));
    }
    expect_validation_error(dir, "not equal to the number of rows");
}

/**************************************/

TEST(DataFrame, VlsVersionError) {
    auto dir = define_test_path("data_frame");

    {
        DataFrameOptions opt(99, { DataFrameColumn("Zuikaku", DataFrameColumnType::VLS) });
        opt.version = "1.0";
        mock_data_frame(dir, opt);
    }
    expect_validation_error(dir, "unsupported type");
}

TEST(DataFrame, VlsHeapError) {
    auto dir = define_test_path("data_frame");

    // Check that we validate the heap.
    {
        DataFrameOptions opt(52, { DataFrameColumn("Atago", DataFrameColumnType::VLS) });
        auto ghandle = mock_data_frame(dir, opt);
        auto dhandle = ghandle.openGroup("data");
        auto xhandle = dhandle.openGroup("0");
        xhandle.unlink("heap");
        xhandle.createDataSet("heap", H5::PredType::NATIVE_INT32, create_hdf5_dataspace(100));
    }
    expect_validation_error(dir, "8-bit unsigned integer");
}

TEST(DataFrame, VlsPointersError) {
    auto dir = define_test_path("data_frame");

    DataFrameOptions opt(48, { DataFrameColumn("Takao", DataFrameColumnType::VLS) });

    {
        auto ghandle = mock_data_frame(dir, opt);
        auto dhandle = ghandle.openGroup("data");
        auto xhandle = dhandle.openGroup("0");
        xhandle.unlink("pointers");
        xhandle.createDataSet("pointers", ritsuko::cvls::define_pointer_datatype<std::uint64_t, std::uint64_t>(), H5S_SCALAR);
    }
    expect_validation_error(dir, "1-dimensional");

    // Check that we validate the pointers.
    {
        auto ghandle = mock_data_frame(dir, opt);
        auto dhandle = ghandle.openGroup("data");
        auto xhandle = dhandle.openGroup("0");
        auto phandle = xhandle.openDataSet("pointers");

        std::vector<ritsuko::cvls::Pointer<std::uint64_t, std::uint64_t> > buffer(opt.num_rows);
        for (hsize_t i = 0; i < opt.num_rows; ++i) {
            buffer[i].offset = 0; 
            buffer[i].length = -1;
        }
        phandle.write(buffer.data(), ritsuko::cvls::define_pointer_datatype<std::uint64_t, std::uint64_t>());
    }
    expect_validation_error(dir, "out of range of the heap");

    // Check that the length is properly passed along.
    {
        auto ghandle = mock_data_frame(dir, opt);
        auto dhandle = ghandle.openGroup("data");
        auto xhandle = dhandle.openGroup("0");
        xhandle.unlink("pointers");

        const hsize_t new_num_rows = 49;
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

    {
        DataFrameOptions opt(82, { DataFrameColumn("Maya", DataFrameColumnType::VLS) });
        auto ghandle = mock_data_frame(dir, opt);
        auto dhandle = ghandle.openGroup("data");
        auto xhandle = dhandle.openGroup("0");
        auto phandle = xhandle.openDataSet("pointers");
        add_hdf5_string_attribute(phandle, "missing-value-placeholder", "foobar");
    }

    test_validate(dir);
    EXPECT_EQ(test_height(dir), 82);
    EXPECT_EQ(test_dimensions(dir), (std::vector<std::size_t>{ 82, 1 }));
}

TEST(DataFrame, VlsMissingError) {
    auto dir = define_test_path("data_frame");

    {
        DataFrameOptions opt(76, { DataFrameColumn("Choukai", DataFrameColumnType::VLS) });
        auto ghandle = mock_data_frame(dir, opt);
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

    DataFrameOptions opt(51, {
        DataFrameColumn("Kanon", DataFrameColumnType::OTHER),
        DataFrameColumn("Chisato", DataFrameColumnType::STRING), // intervening basic column, for some variety.
        DataFrameColumn("Keke", DataFrameColumnType::OTHER)
    });

    {
        mock_data_frame(dir, opt);
        initialize_directory(dir / "other_columns");
        mock_data_frame(dir / "other_columns" / "0", DataFrameOptions(51, { DataFrameColumn("shibuya") }));
        mock_atomic_vector(dir / "other_columns" / "2", AtomicVectorOptions(51));
    }

    test_validate(dir);
    EXPECT_EQ(test_height(dir), 51);
    EXPECT_EQ(test_dimensions(dir), (std::vector<std::size_t>{ 51, 3 }));
}

TEST(DataFrame, OtherError) {
    auto dir = define_test_path("data_frame");

    DataFrameOptions opt(52, {
        DataFrameColumn("Sumire", DataFrameColumnType::OTHER),
        DataFrameColumn("Ren", DataFrameColumnType::OTHER)
    });

    {
        mock_data_frame(dir, opt);
        initialize_directory(dir / "other_columns");
        mock_data_frame(dir / "other_columns" / "0", DataFrameOptions(53, { DataFrameColumn("heanna") }));
        mock_atomic_vector(dir / "other_columns" / "1", AtomicVectorOptions(51));
    }
    expect_validation_error(dir, "height is not equal");

    {
        mock_data_frame(dir, opt);
        initialize_directory(dir / "other_columns");
        initialize_directory_simple(dir / "other_columns" / "0", "superfoobar", "1.0");
    }
    expect_validation_error(dir, "failed to validate column 0");

    {
        mock_data_frame(dir, opt);
        initialize_directory(dir / "other_columns");
        mock_atomic_vector(dir / "other_columns" / "0", AtomicVectorOptions(52));
        mock_atomic_vector(dir / "other_columns" / "1", AtomicVectorOptions(52, AtomicVectorType::BOOLEAN));
        mock_atomic_vector(dir / "other_columns" / "2", AtomicVectorOptions(52, AtomicVectorType::STRING));
    }
    expect_validation_error(dir, "more objects than expected");
}

/**************************************/

TEST(DataFrame, McolsOkay) {
    auto dir = define_test_path("data_frame");

    {
        DataFrameOptions opt(99, { DataFrameColumn("nagisa"), DataFrameColumn("tomoya") });
        mock_data_frame(dir, opt);

        DataFrameOptions metaopt(2, {
            DataFrameColumn("akio", DataFrameColumnType::NUMBER),
            DataFrameColumn("sanae", DataFrameColumnType::STRING),
            DataFrameColumn("ushio", DataFrameColumnType::BOOLEAN)
        });
        mock_data_frame(dir / "column_annotations", metaopt);
    }

    test_validate(dir);
}

TEST(DataFrame, McolsError) {
    auto dir = define_test_path("data_frame");

    // Check that we actually validate the mcols.
    {
        DataFrameOptions opt(99, { DataFrameColumn("nagisa"), DataFrameColumn("tomoya") });
        mock_data_frame(dir, opt);
        mock_data_frame(dir / "column_annotations", DataFrameOptions(3, {}));
    }
    expect_validation_error(dir, "unexpected number of rows");
}

TEST(DataFrame, MetadataOkay) {
    auto dir = define_test_path("data_frame");

    {
        DataFrameOptions opt(13, {
            DataFrameColumn("dango daikazoku", DataFrameColumnType::NUMBER),
            DataFrameColumn("toki wo izuma uta", DataFrameColumnType::STRING),
            DataFrameColumn("the palm of a tiny hand", DataFrameColumnType::BOOLEAN),
            DataFrameColumn("over")
        });
        mock_data_frame(dir, opt);
        mock_simple_list(dir / "other_annotations");
    }

    test_validate(dir);
    EXPECT_EQ(test_height(dir), 13);
    EXPECT_EQ(test_dimensions(dir), (std::vector<std::size_t>{ 13, 4 }));
}

TEST(DataFrame, MetadataError) {
    auto dir = define_test_path("data_frame");

    DataFrameOptions opt(13, {
        DataFrameColumn("cherry blossom petal"),
        DataFrameColumn("roaring ocean")
    });

    // Check that we actually validate the metadata.
    {
        mock_data_frame(dir, opt);
        mock_data_frame(dir / "other_annotations", DataFrameOptions(2, {}));
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
        auto ghandle = mock_data_frame(dir, DataFrameOptions(33, { DataFrameColumn("Aaron") }));
        auto dhandle = ghandle.openGroup("data");
        dhandle.createGroup("foo");
    }
    expect_validation_error(dir, "more objects present");
}
