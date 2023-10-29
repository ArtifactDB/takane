#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "takane/hdf5_data_frame.hpp"

#include <numeric>
#include <string>
#include <vector>

static void create_hdf5_data_frame(const H5::Group& handle, hsize_t num_rows, bool has_row_names, const std::vector<takane::data_frame::ColumnDetails>& columns, int version = 2) {
    {
        hsize_t ncol = columns.size();
        H5::DataSpace dspace(1, &ncol);
        H5::StrType stype(0, H5T_VARIABLE);
        auto dhandle = handle.createDataSet("column_names", stype, dspace);

        std::vector<const char*> column_names;
        column_names.reserve(ncol);
        for (const auto& col : columns) {
            column_names.push_back(col.name.c_str());
        }

        dhandle.write(column_names.data(), stype);
    }

    if (has_row_names) {
        H5::DataSpace dspace(1, &num_rows);
        H5::StrType stype(0, H5T_VARIABLE);
        auto dhandle = handle.createDataSet("row_names", stype, dspace);

        std::vector<std::string> row_names;
        row_names.reserve(num_rows);
        std::vector<const char*> row_names_ptr;
        row_names_ptr.reserve(num_rows);

        for (hsize_t i = 0; i < num_rows; ++i) {
            row_names.push_back(std::to_string(i));
            row_names_ptr.push_back(row_names.back().c_str());
        }

        dhandle.write(row_names_ptr.data(), stype);
    }

    auto ghandle = handle.createGroup("data");
    size_t NC = columns.size();
    for (size_t c = 0; c < NC; ++c) {
        const auto& curcol = columns[c];
        if (curcol.type == takane::data_frame::ColumnType::OTHER) {
            continue;
        }

        std::string colname = std::to_string(c);
        H5::DataSpace dspace(1, &num_rows);

        if (curcol.type == takane::data_frame::ColumnType::INTEGER) {
            std::vector<int> dump(num_rows);
            std::iota(dump.begin(), dump.end(), 0);
            auto dhandle = ghandle.createDataSet(colname, H5::PredType::NATIVE_INT32, dspace);
            dhandle.write(dump.data(), H5::PredType::NATIVE_INT);

        } else if (curcol.type == takane::data_frame::ColumnType::NUMBER) {
            std::vector<double> dump(num_rows);
            std::iota(dump.begin(), dump.end(), 0.5);
            auto dhandle = ghandle.createDataSet(colname, H5::PredType::NATIVE_DOUBLE, dspace);
            dhandle.write(dump.data(), H5::PredType::NATIVE_DOUBLE);

        } else if (curcol.type == takane::data_frame::ColumnType::BOOLEAN) {
            std::vector<int> dump(num_rows);
            for (hsize_t i = 0; i < num_rows; ++i) {
                dump[i] = i % 2;
            }
            auto dhandle = ghandle.createDataSet(colname, H5::PredType::NATIVE_INT8, dspace);
            dhandle.write(dump.data(), H5::PredType::NATIVE_INT);

        } else if (curcol.type == takane::data_frame::ColumnType::STRING) {
            std::vector<std::string> raw_dump(num_rows);
            std::vector<const char*> dump(num_rows);
            for (hsize_t i = 0; i < num_rows; ++i) {
                raw_dump[i] = std::to_string(i);
                dump[i] = raw_dump[i].c_str();
            }
            H5::StrType stype(0, H5T_VARIABLE);
            auto dhandle = ghandle.createDataSet(colname, stype, dspace);
            dhandle.write(dump.data(), stype);

        } else if (curcol.type == takane::data_frame::ColumnType::FACTOR) {
            if (version == 1) {
                std::vector<std::string> choices(curcol.factor_levels->begin(), curcol.factor_levels->end());
                std::vector<const char*> dump(num_rows);
                for (hsize_t i = 0; i < num_rows; ++i) {
                    dump[i] = choices[i % choices.size()].c_str();
                }
                H5::StrType stype(0, H5T_VARIABLE);
                auto dhandle = ghandle.createDataSet(colname, stype, dspace);
                dhandle.write(dump.data(), stype);

            } else {
                int nchoices = curcol.factor_levels->size();
                std::vector<int> dump(num_rows);
                for (hsize_t i = 0; i < num_rows; ++i) {
                    dump[i] = i % nchoices;
                }
                auto dhandle = ghandle.createDataSet(colname, H5::PredType::NATIVE_INT16, dspace);
                dhandle.write(dump.data(), H5::PredType::NATIVE_INT);
            }
        }
    }
}

template<typename ... Args_>
static void expect_error(const std::string& msg, const std::string& path, const std::string& name, Args_&& ... args) {
    H5::H5File handle(path, H5F_ACC_RDONLY);
    auto ghandle = handle.openGroup(name);
    EXPECT_ANY_THROW({
        try {
            takane::hdf5_data_frame::validate(ghandle, std::forward<Args_>(args)...);
        } catch (std::exception& e) {
            EXPECT_THAT(e.what(), ::testing::HasSubstr(msg));
            throw;
        }
    });
}

TEST(Hdf5DataFrame, Rownames) {
    std::string path = "TEST-hdf5_data_frame.h5";
    std::string name = "df";

    takane::hdf5_data_frame::Parameters params;
    params.num_rows = 29;
    params.has_row_names = true;
    auto& columns = params.columns.mutable_ref();
    columns.resize(1);

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup(name);
        create_hdf5_data_frame(ghandle, params.num_rows, true, columns);
    }
    takane::hdf5_data_frame::validate(path, name, params);

    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        auto ghandle = handle.openGroup(name);
        ghandle.unlink("row_names");
    }
    expect_error("expected a 'row_names' dataset", path, name, params);
    params.has_row_names = false;
    takane::hdf5_data_frame::validate(path, name, params);

    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        auto ghandle = handle.openGroup(name);
        ghandle.createGroup("row_names");
    }
    params.has_row_names = true;
    expect_error("expected a 'row_names' dataset", path, name, params);

    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        auto ghandle = handle.openGroup(name);
        ghandle.unlink("row_names");
        ghandle.createDataSet("row_names", H5::PredType::NATIVE_INT, H5S_SCALAR);
    }
    expect_error("string dataset", path, name, params);

    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        auto ghandle = handle.openGroup(name);
        ghandle.unlink("row_names");

        H5::StrType stype(0, H5T_VARIABLE);
        hsize_t dummy = 20;
        H5::DataSpace dspace(1, &dummy);
        ghandle.createDataSet("row_names", stype, dspace);
    }
    expect_error("expected 'row_names' to have length", path, name, params);
}

TEST(Hdf5DataFrame, Colnames) {
    std::string path = "TEST-hdf5_data_frame.h5";
    std::string name = "df";

    takane::hdf5_data_frame::Parameters params;
    params.num_rows = 29;
    auto& columns = params.columns.mutable_ref();
    columns.resize(2);
    columns[0].name = "Aaron";
    columns[1].name = "Barry";

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup(name);
        create_hdf5_data_frame(ghandle, params.num_rows, false, columns);
    }
    takane::hdf5_data_frame::validate(path, name, params);
    
    auto old = columns[1].name;
    columns[1].name = "Charlie";
    expect_error("expected name 'Charlie'", path, name, params);
    columns[1].name = old;

    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        auto ghandle = handle.openGroup(name);
        ghandle.unlink("column_names");
    }
    expect_error("dataset", path, name, params);

    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        auto ghandle = handle.openGroup(name);
        ghandle.createGroup("column_names");
    }    
    expect_error("expected a 'column_names' dataset", path, name, params);

    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        auto ghandle = handle.openGroup(name);
        ghandle.unlink("column_names");
        ghandle.createDataSet("column_names", H5::PredType::NATIVE_INT, H5S_SCALAR);
    }
    expect_error("string dataset", path, name, params);

    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        auto ghandle = handle.openGroup(name);
        ghandle.unlink("column_names");

        H5::StrType stype(0, H5T_VARIABLE);
        hsize_t dummy = 10;
        H5::DataSpace dspace(1, &dummy);
        ghandle.createDataSet("column_names", stype, dspace);
    }
    expect_error("length of 'column_names'", path, name, params);

    columns[1].name = "Aaron";
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup(name);
        create_hdf5_data_frame(ghandle, params.num_rows, false, columns);
    }
    expect_error("duplicated column name", path, name, params);
}

TEST(Hdf5DataFrame, Data) {
    std::string path = "TEST-hdf5_data_frame.h5";
    std::string name = "df";

    takane::hdf5_data_frame::Parameters params;
    params.num_rows = 33;
    auto& columns = params.columns.mutable_ref();
    columns.resize(2);
    columns[0].name = "Aaron";
    columns[1].name = "Barry";

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup(name);
        create_hdf5_data_frame(ghandle, params.num_rows, false, columns);
        ghandle.unlink("data");
    }
    expect_error("'data' group", path, name, params);

    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        auto ghandle = handle.openGroup(name);
        auto dhandle = ghandle.createGroup("data");
        dhandle.createGroup("0");
    }
    expect_error("expected a dataset", path, name, params);

    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        auto ghandle = handle.openGroup(name);
        auto dhandle = ghandle.openGroup("data");
        dhandle.unlink("0");

        hsize_t dummy = 2;
        H5::DataSpace dspace(1, &dummy);
        dhandle.createDataSet("0", H5::PredType::NATIVE_INT32, dspace);
    }
    expect_error("length equal to the number of rows", path, name, params);

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup(name);
        create_hdf5_data_frame(ghandle, params.num_rows, false, columns);
        auto dhandle = ghandle.openGroup("data");
        dhandle.createGroup("foo");
    }
    expect_error("more objects present", path, name, params);

    columns[0].type = takane::data_frame::ColumnType::OTHER;
    columns[1].type = takane::data_frame::ColumnType::OTHER;
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup(name);
        create_hdf5_data_frame(ghandle, params.num_rows, false, columns);
    }
    takane::hdf5_data_frame::validate(path, name, params);
}

TEST(Hdf5DataFrame, Integer) {
    std::string path = "TEST-hdf5_data_frame.h5";
    std::string name = "df";

    takane::hdf5_data_frame::Parameters params;
    auto& columns = params.columns.mutable_ref();
    columns.resize(1);
    columns[0].name = "Aaron";
    columns[0].type = takane::data_frame::ColumnType::INTEGER;
    params.num_rows = 33;

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup(name);
        create_hdf5_data_frame(ghandle, params.num_rows, false, columns);
    }
    takane::hdf5_data_frame::validate(path, name, params);

    hsize_t nrows = params.num_rows;
    H5::DataSpace dspace(1, &nrows);
    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        auto ghandle = handle.openGroup(name);
        auto dhandle = ghandle.openGroup("data");
        dhandle.unlink("0");
        dhandle.createDataSet("0", H5::PredType::NATIVE_DOUBLE, dspace);
    }
    expect_error("integer dataset", path, name, params);

    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        auto ghandle = handle.openGroup(name);
        auto dhandle = ghandle.openGroup("data");
        dhandle.unlink("0");
        dhandle.createDataSet("0", H5::PredType::NATIVE_INT64, dspace);
    }
    expect_error("exceeds the range", path, name, params);

    // Checking the missing value placeholder.
    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        auto ghandle = handle.openGroup(name);
        auto dhandle = ghandle.openGroup("data");
        dhandle.unlink("0");
        auto xhandle = dhandle.createDataSet("0", H5::PredType::NATIVE_INT16, dspace);
        xhandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_INT16, H5S_SCALAR);
    }
    takane::hdf5_data_frame::validate(path, name, params);

    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        auto ghandle = handle.openGroup(name);
        auto dhandle = ghandle.openGroup("data");
        auto xhandle = dhandle.openDataSet("0");
        xhandle.removeAttr("missing-value-placeholder");
        xhandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_INT8, H5S_SCALAR);
    }
    expect_error("same type as", path, name, params);
}

TEST(Hdf5DataFrame, Boolean) {
    std::string path = "TEST-hdf5_data_frame.h5";
    std::string name = "df";

    takane::hdf5_data_frame::Parameters params;
    params.num_rows = 33;
    auto& columns = params.columns.mutable_ref();
    columns.resize(1);
    columns[0].name = "Aaron";
    columns[0].type = takane::data_frame::ColumnType::BOOLEAN;

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup(name);
        create_hdf5_data_frame(ghandle, params.num_rows, false, columns);
    }
    takane::hdf5_data_frame::validate(path, name, params);

    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        auto ghandle = handle.openGroup(name);
        auto dhandle = ghandle.openGroup("data");
        dhandle.unlink("0");
        hsize_t nrows = params.num_rows;
        H5::DataSpace dspace(1, &nrows);
        dhandle.createDataSet("0", H5::PredType::NATIVE_DOUBLE, dspace);
    }
    expect_error("integer dataset", path, name, params);
}

TEST(Hdf5DataFrame, Number) {
    std::string path = "TEST-hdf5_data_frame.h5";
    std::string name = "df";

    takane::hdf5_data_frame::Parameters params;
    params.num_rows = 27;
    auto& columns = params.columns.mutable_ref();
    columns.resize(1);
    columns[0].name = "Aaron";
    columns[0].type = takane::data_frame::ColumnType::NUMBER;

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup(name);
        create_hdf5_data_frame(ghandle, params.num_rows, false, columns);
    }
    takane::hdf5_data_frame::validate(path, name, params);

    hsize_t nrows = params.num_rows;
    H5::DataSpace dspace(1, &nrows);
    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        auto ghandle = handle.openGroup(name);
        auto dhandle = ghandle.openGroup("data");
        dhandle.unlink("0");
        dhandle.createDataSet("0", H5::PredType::NATIVE_INT, dspace);
    }
    expect_error("floating-point dataset", path, name, params);

    // Checking the missing value placeholder.
    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        auto ghandle = handle.openGroup(name);
        auto dhandle = ghandle.openGroup("data");
        dhandle.unlink("0");
        auto xhandle = dhandle.createDataSet("0", H5::PredType::NATIVE_DOUBLE, dspace);
        xhandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_DOUBLE, H5S_SCALAR);
    }
    takane::hdf5_data_frame::validate(path, name, params);

    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        auto ghandle = handle.openGroup(name);
        auto dhandle = ghandle.openGroup("data");
        auto xhandle = dhandle.openDataSet("0");
        xhandle.removeAttr("missing-value-placeholder");
        xhandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_INT8, H5S_SCALAR);
    }
    expect_error("same type as", path, name, params);
}

TEST(Hdf5DataFrame, String) {
    std::string path = "TEST-hdf5_data_frame.h5";
    std::string name = "df";

    takane::hdf5_data_frame::Parameters params;
    params.num_rows = 32;
    auto& columns = params.columns.mutable_ref();
    columns.resize(1);
    columns[0].name = "Aaron";
    columns[0].type = takane::data_frame::ColumnType::STRING;

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup(name);
        create_hdf5_data_frame(ghandle, params.num_rows, false, columns);
    }
    takane::hdf5_data_frame::validate(path, name, params);

    hsize_t nrows = params.num_rows;
    H5::DataSpace dspace(1, &nrows);
    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        auto ghandle = handle.openGroup(name);
        auto dhandle = ghandle.openGroup("data");
        dhandle.unlink("0");
        dhandle.createDataSet("0", H5::PredType::NATIVE_INT, dspace);
    }
    expect_error("string dataset", path, name, params);

    // Checking the missing value placeholder.
    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        auto ghandle = handle.openGroup(name);
        auto dhandle = ghandle.openGroup("data");
        dhandle.unlink("0");

        H5::StrType stype(0, H5T_VARIABLE);
        auto xhandle = dhandle.createDataSet("0", stype, dspace);
        auto ahandle = xhandle.createAttribute("missing-value-placeholder", stype, H5S_SCALAR);
        ahandle.write(stype, std::string("asdasd"));
    }
    takane::hdf5_data_frame::validate(path, name, params);

    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        auto ghandle = handle.openGroup(name);
        auto dhandle = ghandle.openGroup("data");
        auto xhandle = dhandle.openDataSet("0");
        xhandle.removeAttr("missing-value-placeholder");
        xhandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_INT8, H5S_SCALAR);
    }
    expect_error("same type class as", path, name, params);
}

TEST(Hdf5DataFrame, StringDate) {
    std::string path = "TEST-hdf5_data_frame.h5";
    std::string name = "df";

    takane::hdf5_data_frame::Parameters params;
    params.num_rows = 32;
    auto& columns = params.columns.mutable_ref();
    columns.resize(1);
    columns[0].name = "Aaron";
    columns[0].type = takane::data_frame::ColumnType::STRING;
    columns[0].format = takane::data_frame::StringFormat::DATE;

    const char* exemplar = "2023-11-02";
    hsize_t nrows = params.num_rows;
    std::vector<const char*> dump(nrows, exemplar);
    H5::StrType stype(0, H5T_VARIABLE);
    H5::DataSpace dspace(1, &nrows);

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup(name);
        create_hdf5_data_frame(ghandle, params.num_rows, false, columns);
        auto dhandle = ghandle.openGroup("data");
        dhandle.unlink("0");
        auto xhandle = dhandle.createDataSet("0", stype, dspace);
        xhandle.write(dump.data(), stype);
    }
    takane::hdf5_data_frame::validate(path, name, params);

    const char* violator = "asdasd";
    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        auto ghandle = handle.openGroup(name);
        auto dhandle = ghandle.openGroup("data");
        dhandle.unlink("0");
        auto xhandle = dhandle.createDataSet("0", stype, dspace);
        dump.back() = violator;
        xhandle.write(dump.data(), stype);
    }
    expect_error("date-formatted", path, name, params);

    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        auto ghandle = handle.openGroup(name);
        auto dhandle = ghandle.openGroup("data");
        auto xhandle = dhandle.openDataSet("0");
        auto ahandle = xhandle.createAttribute("missing-value-placeholder", stype, H5S_SCALAR);
        ahandle.write(stype, std::string(violator));
    }
    takane::hdf5_data_frame::validate(path, name, params);
}

TEST(Hdf5DataFrame, StringDateTime) {
    std::string path = "TEST-hdf5_data_frame.h5";
    std::string name = "df";

    takane::hdf5_data_frame::Parameters params;
    params.num_rows = 32;
    auto& columns = params.columns.mutable_ref();
    columns.resize(1);
    columns[0].name = "Aaron";
    columns[0].type = takane::data_frame::ColumnType::STRING;
    columns[0].format = takane::data_frame::StringFormat::DATE_TIME;

    const char* exemplar = "2023-11-02T23:01:02Z";
    hsize_t nrows = params.num_rows;
    std::vector<const char*> dump(nrows, exemplar);
    H5::StrType stype(0, H5T_VARIABLE);
    H5::DataSpace dspace(1, &nrows);

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup(name);
        create_hdf5_data_frame(ghandle, params.num_rows, false, columns);
        auto dhandle = ghandle.openGroup("data");
        dhandle.unlink("0");
        auto xhandle = dhandle.createDataSet("0", stype, dspace);
        xhandle.write(dump.data(), stype);
    }
    takane::hdf5_data_frame::validate(path, name, params);

    const char* violator = "asdasd";
    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        auto ghandle = handle.openGroup(name);
        auto dhandle = ghandle.openGroup("data");
        dhandle.unlink("0");
        auto xhandle = dhandle.createDataSet("0", stype, dspace);
        dump.back() = violator;
        xhandle.write(dump.data(), stype);
    }
    expect_error("date/time-formatted", path, name, params);

    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        auto ghandle = handle.openGroup(name);
        auto dhandle = ghandle.openGroup("data");
        auto xhandle = dhandle.openDataSet("0");
        auto ahandle = xhandle.createAttribute("missing-value-placeholder", stype, H5S_SCALAR);
        ahandle.write(stype, std::string(violator));
    }
    takane::hdf5_data_frame::validate(path, name, params);
}

TEST(Hdf5DataFrame, FactorVersion1) {
    std::string path = "TEST-hdf5_data_frame.h5";
    std::string name = "df";

    takane::hdf5_data_frame::Parameters params;
    params.num_rows = 32;
    auto& columns = params.columns.mutable_ref();
    columns.resize(1);
    columns[0].name = "Aaron";
    columns[0].type = takane::data_frame::ColumnType::FACTOR;
    std::vector<std::string> levels{ "kanon", "chisato", "sumire", "ren", "keke" };
    columns[0].factor_levels.mutable_ref().insert(levels.begin(), levels.end());

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup(name);
        create_hdf5_data_frame(ghandle, params.num_rows, false, columns, /* version = */ 1);
    }
    params.df_version = 1;
    params.hdf5_version = 1;
    takane::hdf5_data_frame::validate(path, name, params);
    columns[0].factor_levels.mutable_ref().erase("chisato");
    expect_error("contains 'chisato'", path, name, params);

    hsize_t nrows = params.num_rows;
    H5::DataSpace dspace(1, &nrows);
    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        auto ghandle = handle.openGroup(name);
        auto dhandle = ghandle.openGroup("data");
        dhandle.unlink("0");
        dhandle.createDataSet("0", H5::PredType::NATIVE_DOUBLE, dspace);
    }
    expect_error("string dataset", path, name, params);

    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        auto ghandle = handle.openGroup(name);
        auto dhandle = ghandle.openGroup("data");
        dhandle.unlink("0");

        H5::StrType stype(0, H5T_VARIABLE);
        auto xhandle = dhandle.createDataSet("0", stype, dspace);
        const char* missing = "chisato";
        std::vector<const char*> dump(nrows, missing);
        xhandle.write(dump.data(), stype);

        auto ahandle = xhandle.createAttribute("missing-value-placeholder", stype, H5S_SCALAR); // rescues the missing values.
        ahandle.write(stype, std::string(missing));
    }
    takane::hdf5_data_frame::validate(path, name, params);
}

TEST(Hdf5DataFrame, FactorVersion2) {
    std::string path = "TEST-hdf5_data_frame.h5";
    std::string name = "df";

    takane::hdf5_data_frame::Parameters params;
    params.num_rows = 32;
    auto& columns = params.columns.mutable_ref();
    columns.resize(1);
    columns[0].name = "Aaron";
    columns[0].type = takane::data_frame::ColumnType::FACTOR;
    auto& factor_levels = columns[0].factor_levels.mutable_ref();
    factor_levels.insert("kanon");
    factor_levels.insert("chisato");
    factor_levels.insert("sumire");
    factor_levels.insert("ren");
    factor_levels.insert("keke");

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup(name);
        create_hdf5_data_frame(ghandle, params.num_rows, false, columns);
    }
    takane::hdf5_data_frame::validate(path, name, params);
    factor_levels.erase("chisato");
    expect_error("less than the number of levels", path, name, params);

    hsize_t nrows = params.num_rows;
    H5::DataSpace dspace(1, &nrows);
    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        auto ghandle = handle.openGroup(name);
        auto dhandle = ghandle.openGroup("data");
        dhandle.unlink("0");
        dhandle.createDataSet("0", H5::PredType::NATIVE_DOUBLE, dspace);
    }
    expect_error("integer dataset", path, name, params);

    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        auto ghandle = handle.openGroup(name);
        auto dhandle = ghandle.openGroup("data");
        dhandle.unlink("0");
        dhandle.createDataSet("0", H5::PredType::NATIVE_INT64, dspace);
    }
    expect_error("exceeds the range", path, name, params);

    // Using -1 as a placeholder value.
    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        auto ghandle = handle.openGroup(name);
        auto dhandle = ghandle.openGroup("data");
        dhandle.unlink("0");

        auto xhandle = dhandle.createDataSet("0", H5::PredType::NATIVE_INT16, dspace);
        std::vector<int> replacement(nrows, -1);
        xhandle.write(replacement.data(), H5::PredType::NATIVE_INT);
    }
    expect_error("non-negative", path, name, params);

    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        auto ghandle = handle.openGroup(name);
        auto dhandle = ghandle.openGroup("data");
        auto xhandle = dhandle.openDataSet("0");
        auto ahandle = xhandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_INT16, H5S_SCALAR);
        int val = -1;
        ahandle.write(H5::PredType::NATIVE_INT, &val); 
    }
    takane::hdf5_data_frame::validate(path, name, params); // rescues the negative values.

    // Using -2^31 as the placeholder, to check legacy loaders.
    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        auto ghandle = handle.openGroup(name);
        auto dhandle = ghandle.openGroup("data");
        dhandle.unlink("0");

        auto xhandle = dhandle.createDataSet("0", H5::PredType::NATIVE_INT32, dspace);
        std::vector<int> replacement(nrows, -2147483648);
        xhandle.write(replacement.data(), H5::PredType::NATIVE_INT);
    }
    expect_error("non-negative", path, name, params);
    params.hdf5_version = 1;
    takane::hdf5_data_frame::validate(path, name, params);
}
