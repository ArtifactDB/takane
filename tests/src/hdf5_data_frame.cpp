#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "takane/hdf5_data_frame.hpp"

#include <numeric>
#include <string>
#include <vector>

struct Hdf5DataFrameTest : public ::testing::TestWithParam<int> {
    Hdf5DataFrameTest() {
        path = "TEST-hdf5_data_frame.h5";
        name = "df";
    }
    std::string path, name;

public:
    template<class Handle>
    static void attach_type(const Handle& handle, const std::string& type) {
        H5::StrType stype(0, H5T_VARIABLE);
        auto attr = handle.createAttribute("type", stype, H5S_SCALAR);
        attr.write(stype, type);
    }

    template<class Handle>
    static void attach_format(const Handle& handle, const std::string& format) {
        H5::StrType stype(0, H5T_VARIABLE);
        auto attr = handle.createAttribute("format", stype, H5S_SCALAR);
        attr.write(stype, format);
    }

    static H5::DataSet spawn_column(const H5::Group& handle, const std::string& name, hsize_t num_rows, int version, const H5::DataType& dtype, const std::string& ntype) {
        H5::DataSpace dspace(1, &num_rows);
        auto out = handle.createDataSet(name, dtype, dspace);
        if (version >= 3) {
            attach_type(out, ntype);
        }
        return out;
    }

    static H5::DataSet spawn_integer_column(const H5::Group& handle, const std::string& name, hsize_t num_rows, int version, const H5::DataType& dtype = H5::PredType::NATIVE_INT32) {
        return spawn_column(handle, name, num_rows, version, dtype, "integer");
    }

    static H5::DataSet spawn_number_column(const H5::Group& handle, const std::string& name, hsize_t num_rows, int version, const H5::DataType& dtype = H5::PredType::NATIVE_DOUBLE) {
        return spawn_column(handle, name, num_rows, version, dtype, "number");
    }

    static H5::DataSet spawn_string_column(const H5::Group& handle, const std::string& name, hsize_t num_rows, int version, const H5::DataType& dtype) {
        return spawn_column(handle, name, num_rows, version, dtype, "string");
    }

    static H5::DataSet spawn_boolean_column(const H5::Group& handle, const std::string& name, hsize_t num_rows, int version, const H5::DataType& dtype = H5::PredType::NATIVE_INT8) {
        return spawn_column(handle, name, num_rows, version, dtype, "boolean");
    }

    template<class Container_>
    static std::vector<const char*> pointerize_strings(const Container_& x) {
        std::vector<const char*> output;
        for (auto start = x.begin(), end = x.end(); start != end; ++start) {
            output.push_back(start->c_str());
        }
        return output;
    }

public:
    static void create_hdf5_data_frame(const H5::Group& handle, hsize_t num_rows, bool has_row_names, const std::vector<takane::data_frame::ColumnDetails>& columns, int version) {
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

        if (version >= 3) {
            auto attr = handle.createAttribute("num_rows", H5::PredType::NATIVE_UINT32, H5S_SCALAR);
            attr.write(H5::PredType::NATIVE_HSIZE, &num_rows);

            H5::StrType stype(0, H5T_VARIABLE);
            auto attr2 = handle.createAttribute("version", stype, H5S_SCALAR);
            attr2.write(stype, std::string("1.0"));
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
                auto dhandle = spawn_integer_column(ghandle, colname, num_rows, version);
                std::vector<int> dump(num_rows);
                std::iota(dump.begin(), dump.end(), 0);
                dhandle.write(dump.data(), H5::PredType::NATIVE_INT);

            } else if (curcol.type == takane::data_frame::ColumnType::NUMBER) {
                std::vector<double> dump(num_rows);
                std::iota(dump.begin(), dump.end(), 0.5);
                auto dhandle = spawn_number_column(ghandle, colname, num_rows, version);
                dhandle.write(dump.data(), H5::PredType::NATIVE_DOUBLE);

            } else if (curcol.type == takane::data_frame::ColumnType::BOOLEAN) {
                std::vector<int> dump(num_rows);
                for (hsize_t i = 0; i < num_rows; ++i) {
                    dump[i] = i % 2;
                }
                auto dhandle = spawn_boolean_column(ghandle, colname, num_rows, version);
                dhandle.write(dump.data(), H5::PredType::NATIVE_INT);

            } else if (curcol.type == takane::data_frame::ColumnType::STRING) {
                std::vector<std::string> raw_dump(num_rows);
                for (hsize_t i = 0; i < num_rows; ++i) {
                    raw_dump[i] = std::to_string(i);
                }
                H5::StrType stype(0, H5T_VARIABLE);
                auto dhandle = spawn_string_column(ghandle, colname, num_rows, version, stype);
                auto dump = pointerize_strings(raw_dump);
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

                } else if (version == 2) {
                    int nchoices = curcol.factor_levels->size();
                    std::vector<int> dump(num_rows);
                    for (hsize_t i = 0; i < num_rows; ++i) {
                        dump[i] = i % nchoices;
                    }
                    auto dhandle = ghandle.createDataSet(colname, H5::PredType::NATIVE_INT16, dspace);
                    dhandle.write(dump.data(), H5::PredType::NATIVE_INT);

                } else {
                    auto dhandle = ghandle.createGroup(colname);
                    attach_type(dhandle, "factor");

                    hsize_t nchoices = 0;
                    {
                        auto dump = pointerize_strings(*(curcol.factor_levels));
                        nchoices = dump.size();
                        H5::StrType stype(0, H5T_VARIABLE);
                        H5::DataSpace dspace(1, &nchoices);
                        auto lhandle = dhandle.createDataSet("levels", stype, dspace);
                        lhandle.write(dump.data(), stype);
                    }

                    std::vector<int> codes(num_rows);
                    for (hsize_t i = 0; i < num_rows; ++i) {
                        codes[i] = i % nchoices;
                    }
                    auto chandle = dhandle.createDataSet("codes", H5::PredType::NATIVE_INT16, dspace);
                    chandle.write(codes.data(), H5::PredType::NATIVE_INT);
                }
            }
        }
    }

public:
    template<typename ... Args_>
    static void expect_error(const std::string& msg, Args_&& ... args) {
        EXPECT_ANY_THROW({
            try {
                takane::hdf5_data_frame::validate(std::forward<Args_>(args)...);
            } catch (std::exception& e) {
                EXPECT_THAT(e.what(), ::testing::HasSubstr(msg));
                throw;
            }
        });
    }
};

TEST_P(Hdf5DataFrameTest, Rownames) {
    takane::hdf5_data_frame::Parameters params(name);
    params.num_rows = 29;
    params.has_row_names = true;
    auto& columns = params.columns.mutable_ref();
    columns.resize(1);

    auto version = GetParam();
    params.df_version = version;
    params.hdf5_version = version;

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup(name);
        create_hdf5_data_frame(ghandle, params.num_rows, true, columns, version);
    }
    takane::hdf5_data_frame::validate(path.c_str(), params);

    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        auto ghandle = handle.openGroup(name);
        ghandle.unlink("row_names");
    }
    expect_error("expected a 'row_names' dataset", path.c_str(), params);
    params.has_row_names = false;
    takane::hdf5_data_frame::validate(path.c_str(), params);

    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        auto ghandle = handle.openGroup(name);
        ghandle.createGroup("row_names");
    }
    params.has_row_names = true;
    expect_error("expected a 'row_names' dataset", path.c_str(), params);

    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        auto ghandle = handle.openGroup(name);
        ghandle.unlink("row_names");
        ghandle.createDataSet("row_names", H5::PredType::NATIVE_INT, H5S_SCALAR);
    }
    expect_error("string dataset", path.c_str(), params);

    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        auto ghandle = handle.openGroup(name);
        ghandle.unlink("row_names");

        H5::StrType stype(0, H5T_VARIABLE);
        hsize_t dummy = 20;
        H5::DataSpace dspace(1, &dummy);
        ghandle.createDataSet("row_names", stype, dspace);
    }
    expect_error("expected 'row_names' to have length", path.c_str(), params);
}

TEST_P(Hdf5DataFrameTest, Colnames) {
    takane::hdf5_data_frame::Parameters params(name);
    params.num_rows = 29;
    auto& columns = params.columns.mutable_ref();
    columns.resize(2);
    columns[0].name = "Aaron";
    columns[1].name = "Barry";

    auto version = GetParam();
    params.df_version = version;
    params.hdf5_version = version;

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup(name);
        create_hdf5_data_frame(ghandle, params.num_rows, false, columns, version);
    }
    takane::hdf5_data_frame::validate(path.c_str(), params);
    
    auto old = columns[1].name;
    columns[1].name = "Charlie";
    expect_error("expected name 'Charlie'", path.c_str(), params);
    columns[1].name = old;

    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        auto ghandle = handle.openGroup(name);
        ghandle.unlink("column_names");
    }
    expect_error("dataset", path.c_str(), params);

    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        auto ghandle = handle.openGroup(name);
        ghandle.createGroup("column_names");
    }
    expect_error("expected a 'column_names' dataset", path.c_str(), params);

    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        auto ghandle = handle.openGroup(name);
        ghandle.unlink("column_names");
        ghandle.createDataSet("column_names", H5::PredType::NATIVE_INT, H5S_SCALAR);
    }
    expect_error("string dataset", path.c_str(), params);

    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        auto ghandle = handle.openGroup(name);
        ghandle.unlink("column_names");

        H5::StrType stype(0, H5T_VARIABLE);
        hsize_t dummy = 10;
        H5::DataSpace dspace(1, &dummy);
        ghandle.createDataSet("column_names", stype, dspace);
    }
    expect_error("length of 'column_names'", path.c_str(), params);

    columns[1].name = "Aaron";
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup(name);
        create_hdf5_data_frame(ghandle, params.num_rows, false, columns, version);
    }
    expect_error("duplicated column name", path.c_str(), params);
}

TEST_P(Hdf5DataFrameTest, General) {
    takane::hdf5_data_frame::Parameters params(name);
    params.num_rows = 33;
    auto& columns = params.columns.mutable_ref();
    columns.resize(2);
    columns[0].name = "Aaron";
    columns[1].name = "Barry";

    auto version = GetParam();
    params.df_version = version;
    params.hdf5_version = version;

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
    }
    expect_error("'" + name + "' group", path.c_str(), params);

    H5::StrType stype(0, H5T_VARIABLE);
    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        auto ghandle = handle.createGroup(name);
        auto attr = ghandle.createAttribute("version", stype, H5S_SCALAR);
        attr.write(stype, std::string("2.0"));
    }
    expect_error("unsupported version", path.c_str(), params);

    if (version >= 3) {
        {
            H5::H5File handle(path, H5F_ACC_RDWR);
            auto ghandle = handle.openGroup(name);
            ghandle.removeAttr("version");
            auto attr = ghandle.createAttribute("version", stype, H5S_SCALAR);
            attr.write(stype, std::string("1.0"));
            ghandle.createAttribute("num_rows", H5::PredType::NATIVE_INT8, H5S_SCALAR);
        }
        expect_error("64-bit unsigned", path.c_str(), params);

        {
            H5::H5File handle(path, H5F_ACC_RDWR);
            auto ghandle = handle.openGroup(name);
            ghandle.removeAttr("num_rows");
            ghandle.createAttribute("num_rows", H5::PredType::NATIVE_UINT8, H5S_SCALAR);
        }
        expect_error("inconsistent number", path.c_str(), params);
    }

}

TEST_P(Hdf5DataFrameTest, Data) {
    takane::hdf5_data_frame::Parameters params(name);
    params.num_rows = 33;
    auto& columns = params.columns.mutable_ref();
    columns.resize(2);
    columns[0].name = "Aaron";
    columns[1].name = "Barry";

    auto version = GetParam();
    params.df_version = version;
    params.hdf5_version = version;

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup(name);
        create_hdf5_data_frame(ghandle, params.num_rows, false, columns, version);
        ghandle.unlink("data");
    }
    expect_error("'df/data' group", path.c_str(), params);

    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        auto ghandle = handle.openGroup(name);
        auto dhandle = ghandle.createGroup("data");
        dhandle.createGroup("0");
    }
    if (version <= 2) {
        expect_error("expected a dataset", path.c_str(), params);
    } else {
        expect_error("only factor columns", path.c_str(), params);
    }

    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        auto ghandle = handle.openGroup(name);
        auto dhandle = ghandle.openGroup("data");
        dhandle.unlink("0");
        spawn_integer_column(dhandle, "0", 2, version);
    }
    expect_error("length equal to the number of rows", path.c_str(), params);

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup(name);
        create_hdf5_data_frame(ghandle, params.num_rows, false, columns, version);
        auto dhandle = ghandle.openGroup("data");
        dhandle.createGroup("foo");
    }
    expect_error("more objects present", path.c_str(), params);
}

TEST_P(Hdf5DataFrameTest, Other) {
    takane::hdf5_data_frame::Parameters params(name);
    params.num_rows = 33;
    auto& columns = params.columns.mutable_ref();
    columns.resize(2);
    columns[0].name = "Aaron";
    columns[0].type = takane::data_frame::ColumnType::OTHER;
    columns[1].name = "Barry";
    columns[1].type = takane::data_frame::ColumnType::OTHER;

    auto version = GetParam();
    params.df_version = version;
    params.hdf5_version = version;

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup(name);
        create_hdf5_data_frame(ghandle, params.num_rows, false, columns, version);
    }
    takane::hdf5_data_frame::validate(path.c_str(), params);

    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        auto ghandle = handle.openGroup(name);
        auto dhandle = ghandle.openGroup("data");
        hsize_t nr = params.num_rows;
        H5::DataSpace dspace(1, &nr); 
        dhandle.createDataSet("0", H5::PredType::NATIVE_INT, dspace);
    }
    expect_error("'other'", path.c_str(), params);
}

TEST_P(Hdf5DataFrameTest, Integer) {
    takane::hdf5_data_frame::Parameters params(name);
    auto& columns = params.columns.mutable_ref();
    columns.resize(1);
    columns[0].name = "Aaron";
    columns[0].type = takane::data_frame::ColumnType::INTEGER;
    params.num_rows = 33;

    auto version = GetParam();
    params.df_version = version;
    params.hdf5_version = version;

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup(name);
        create_hdf5_data_frame(ghandle, params.num_rows, false, columns, version);
    }
    takane::hdf5_data_frame::validate(path.c_str(), params);

    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        auto ghandle = handle.openGroup(name);
        auto dhandle = ghandle.openGroup("data");
        dhandle.unlink("0");
        spawn_integer_column(dhandle, "0", params.num_rows, version, H5::PredType::NATIVE_DOUBLE);
    }
    expect_error("expected integer column", path.c_str(), params);

    if (version >= 3) {
        {
            H5::H5File handle(path, H5F_ACC_RDWR);
            auto ghandle = handle.openGroup(name);
            auto dhandle = ghandle.openGroup("data");
            dhandle.unlink("0");
            spawn_boolean_column(dhandle, "0", params.num_rows, version);
        }
        expect_error("'type' attribute set to 'integer'", path.c_str(), params);
    }

    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        auto ghandle = handle.openGroup(name);
        auto dhandle = ghandle.openGroup("data");
        dhandle.unlink("0");
        spawn_integer_column(dhandle, "0", params.num_rows, version, H5::PredType::NATIVE_INT64);
    }
    expect_error("32-bit signed integer", path.c_str(), params);

    // Checking the missing value placeholder.
    if (version >= 2) {
        {
            H5::H5File handle(path, H5F_ACC_RDWR);
            auto ghandle = handle.openGroup(name);
            auto dhandle = ghandle.openGroup("data");
            dhandle.unlink("0");
            auto xhandle = spawn_integer_column(dhandle, "0", params.num_rows, version, H5::PredType::NATIVE_INT16);
            xhandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_INT16, H5S_SCALAR);
        }
        takane::hdf5_data_frame::validate(path.c_str(), params);

        {
            H5::H5File handle(path, H5F_ACC_RDWR);
            auto ghandle = handle.openGroup(name);
            auto dhandle = ghandle.openGroup("data");
            auto xhandle = dhandle.openDataSet("0");
            xhandle.removeAttr("missing-value-placeholder");
            xhandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_INT8, H5S_SCALAR);
        }
        expect_error("same type as", path.c_str(), params);
    }
}

TEST_P(Hdf5DataFrameTest, Boolean) {
    takane::hdf5_data_frame::Parameters params(name);
    params.num_rows = 33;
    auto& columns = params.columns.mutable_ref();
    columns.resize(1);
    columns[0].name = "Aaron";
    columns[0].type = takane::data_frame::ColumnType::BOOLEAN;

    auto version = GetParam();
    params.df_version = version;
    params.hdf5_version = version;

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup(name);
        create_hdf5_data_frame(ghandle, params.num_rows, false, columns, version);
    }
    takane::hdf5_data_frame::validate(path.c_str(), params);

    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        auto ghandle = handle.openGroup(name);
        auto dhandle = ghandle.openGroup("data");
        dhandle.unlink("0");
        spawn_boolean_column(dhandle, "0", params.num_rows, version, H5::PredType::NATIVE_DOUBLE);
    }
    expect_error("expected boolean column", path.c_str(), params);

    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        auto ghandle = handle.openGroup(name);
        auto dhandle = ghandle.openGroup("data");
        dhandle.unlink("0");
        spawn_boolean_column(dhandle, "0", params.num_rows, version, H5::PredType::NATIVE_INT64);
    }
    expect_error("32-bit signed integer", path.c_str(), params);

    if (version >= 3) {
        {
            H5::H5File handle(path, H5F_ACC_RDWR);
            auto ghandle = handle.openGroup(name);
            auto dhandle = ghandle.openGroup("data");
            dhandle.unlink("0");
            spawn_integer_column(dhandle, "0", params.num_rows, version);
        }
        expect_error("'type' attribute set to 'boolean'", path.c_str(), params);
    }

    // Checking the missing value placeholder.
    if (version >= 2) {
        {
            H5::H5File handle(path, H5F_ACC_RDWR);
            auto ghandle = handle.openGroup(name);
            auto dhandle = ghandle.openGroup("data");
            dhandle.unlink("0");
            auto xhandle = spawn_boolean_column(dhandle, "0", params.num_rows, version);
            xhandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_INT8, H5S_SCALAR);
        }
        takane::hdf5_data_frame::validate(path.c_str(), params);

        {
            H5::H5File handle(path, H5F_ACC_RDWR);
            auto ghandle = handle.openGroup(name);
            auto dhandle = ghandle.openGroup("data");
            auto xhandle = dhandle.openDataSet("0");
            xhandle.removeAttr("missing-value-placeholder");
            xhandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_UINT16, H5S_SCALAR);
        }
        expect_error("same type as", path.c_str(), params);
    }
}

TEST_P(Hdf5DataFrameTest, Number) {
    auto version = GetParam();

    takane::hdf5_data_frame::Parameters params(name);
    params.num_rows = 27;
    auto& columns = params.columns.mutable_ref();
    columns.resize(1);
    columns[0].name = "Aaron";
    columns[0].type = takane::data_frame::ColumnType::NUMBER;

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup(name);
        create_hdf5_data_frame(ghandle, params.num_rows, false, columns, version);
    }
    takane::hdf5_data_frame::validate(path.c_str(), params);

    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        auto ghandle = handle.openGroup(name);
        auto dhandle = ghandle.openGroup("data");
        dhandle.unlink("0");
        spawn_number_column(dhandle, "0", params.num_rows, version, H5::PredType::NATIVE_INT);
    }
    if (version <= 2) {
        expect_error("floating-point dataset", path.c_str(), params);
    } else {
        takane::hdf5_data_frame::validate(path.c_str(), params);
    }

    if (version >= 3) {
        {
            H5::H5File handle(path, H5F_ACC_RDWR);
            auto ghandle = handle.openGroup(name);
            auto dhandle = ghandle.openGroup("data");
            dhandle.unlink("0");
            spawn_integer_column(dhandle, "0", params.num_rows, version, H5::PredType::NATIVE_DOUBLE);
        }
        expect_error("'type' attribute set to 'number'", path.c_str(), params);

        {
            H5::H5File handle(path, H5F_ACC_RDWR);
            auto ghandle = handle.openGroup(name);
            auto dhandle = ghandle.openGroup("data");
            dhandle.unlink("0");
            spawn_number_column(dhandle, "0", params.num_rows, version, H5::PredType::NATIVE_INT64);
        }
        expect_error("64-bit float", path.c_str(), params);
    }

    // Checking the missing value placeholder.
    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        auto ghandle = handle.openGroup(name);
        auto dhandle = ghandle.openGroup("data");
        dhandle.unlink("0");
        auto xhandle = spawn_number_column(dhandle, "0", params.num_rows, version);
        xhandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_DOUBLE, H5S_SCALAR);
    }
    takane::hdf5_data_frame::validate(path.c_str(), params);

    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        auto ghandle = handle.openGroup(name);
        auto dhandle = ghandle.openGroup("data");
        auto xhandle = dhandle.openDataSet("0");
        xhandle.removeAttr("missing-value-placeholder");
        xhandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_INT8, H5S_SCALAR);
    }
    expect_error("same type as", path.c_str(), params);
}

TEST_P(Hdf5DataFrameTest, String) {
    takane::hdf5_data_frame::Parameters params(name);
    params.num_rows = 32;
    auto& columns = params.columns.mutable_ref();
    columns.resize(1);
    columns[0].name = "Aaron";
    columns[0].type = takane::data_frame::ColumnType::STRING;

    auto version = GetParam();
    params.df_version = version;
    params.hdf5_version = version;

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup(name);
        create_hdf5_data_frame(ghandle, params.num_rows, false, columns, version);
    }
    takane::hdf5_data_frame::validate(path.c_str(), params);

    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        auto ghandle = handle.openGroup(name);
        auto dhandle = ghandle.openGroup("data");
        dhandle.unlink("0");
        spawn_string_column(dhandle, "0", params.num_rows, version, H5::PredType::NATIVE_INT);
    }
    expect_error("string dataset", path.c_str(), params);

    if (version >= 3) {
        {
            H5::H5File handle(path, H5F_ACC_RDWR);
            auto ghandle = handle.openGroup(name);
            auto dhandle = ghandle.openGroup("data");
            dhandle.unlink("0");
            spawn_integer_column(dhandle, "0", params.num_rows, version, H5::StrType(0, H5T_VARIABLE));
        }
        expect_error("'type' attribute set to 'string'", path.c_str(), params);

        {
            H5::H5File handle(path, H5F_ACC_RDWR);
            auto ghandle = handle.openGroup(name);
            auto dhandle = ghandle.openGroup("data");
            dhandle.unlink("0");
            auto xhandle = spawn_string_column(dhandle, "0", params.num_rows, version, H5::StrType(0, H5T_VARIABLE));
            attach_format(xhandle, "whee");
        }
        expect_error("should be 'none'", path.c_str(), params);

        {
            H5::H5File handle(path, H5F_ACC_RDWR);
            auto ghandle = handle.openGroup(name);
            auto dhandle = ghandle.openGroup("data");
            auto xhandle = dhandle.openDataSet("0");
            xhandle.removeAttr("format");
            attach_format(xhandle, "none");
        }
        takane::hdf5_data_frame::validate(path.c_str(), params);
    }

    // Checking the missing value placeholder.
    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        auto ghandle = handle.openGroup(name);
        auto dhandle = ghandle.openGroup("data");
        dhandle.unlink("0");

        H5::StrType stype(0, H5T_VARIABLE);
        auto xhandle = spawn_string_column(dhandle, "0", params.num_rows, version, stype);
        auto ahandle = xhandle.createAttribute("missing-value-placeholder", stype, H5S_SCALAR);
        ahandle.write(stype, std::string("asdasd"));
    }
    takane::hdf5_data_frame::validate(path.c_str(), params);

    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        auto ghandle = handle.openGroup(name);
        auto dhandle = ghandle.openGroup("data");
        auto xhandle = dhandle.openDataSet("0");
        xhandle.removeAttr("missing-value-placeholder");
        xhandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_INT8, H5S_SCALAR);
    }
    expect_error("same type class as", path.c_str(), params);
}

TEST_P(Hdf5DataFrameTest, StringDate) {
    takane::hdf5_data_frame::Parameters params(name);
    params.num_rows = 32;
    auto& columns = params.columns.mutable_ref();
    columns.resize(1);
    columns[0].name = "Aaron";
    columns[0].type = takane::data_frame::ColumnType::STRING;
    columns[0].string_format = takane::data_frame::StringFormat::DATE;

    auto version = GetParam();
    params.df_version = version;
    params.hdf5_version = version;

    const char* exemplar = "2023-11-02";
    std::vector<const char*> dump(params.num_rows, exemplar);
    H5::StrType stype(0, H5T_VARIABLE);

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup(name);
        create_hdf5_data_frame(ghandle, params.num_rows, false, columns, version);
        auto dhandle = ghandle.openGroup("data");
        dhandle.unlink("0");
        auto xhandle = spawn_string_column(dhandle, "0", params.num_rows, version, stype);
        xhandle.write(dump.data(), stype);
        if (version >= 3) {
            attach_format(xhandle, "date");
        }
    }
    takane::hdf5_data_frame::validate(path.c_str(), params);

    if (version >= 3) {
        {
            H5::H5File handle(path, H5F_ACC_RDWR);
            auto ghandle = handle.openGroup(name);
            auto dhandle = ghandle.openGroup("data");
            dhandle.unlink("0");
            auto xhandle = spawn_string_column(dhandle, "0", params.num_rows, version, stype);
            attach_format(xhandle, "none");
            xhandle.write(dump.data(), stype);
        }
        expect_error("'format' attribute set to 'date'", path.c_str(), params);
    }

    const char* violator = "asdasd";
    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        auto ghandle = handle.openGroup(name);
        auto dhandle = ghandle.openGroup("data");
        dhandle.unlink("0");
        auto xhandle = spawn_string_column(dhandle, "0", params.num_rows, version, stype);
        if (version >= 3) {
            attach_format(xhandle, "date");
        }
        dump.back() = violator;
        xhandle.write(dump.data(), stype);
    }
    expect_error("date-formatted", path.c_str(), params);

    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        auto ghandle = handle.openGroup(name);
        auto dhandle = ghandle.openGroup("data");
        auto xhandle = dhandle.openDataSet("0");
        auto ahandle = xhandle.createAttribute("missing-value-placeholder", stype, H5S_SCALAR);
        ahandle.write(stype, std::string(violator));
    }
    takane::hdf5_data_frame::validate(path.c_str(), params);
}

TEST_P(Hdf5DataFrameTest, StringDateTime) {
    takane::hdf5_data_frame::Parameters params(name);
    params.num_rows = 32;
    auto& columns = params.columns.mutable_ref();
    columns.resize(1);
    columns[0].name = "Aaron";
    columns[0].type = takane::data_frame::ColumnType::STRING;
    columns[0].string_format = takane::data_frame::StringFormat::DATE_TIME;

    auto version = GetParam();
    params.df_version = version;
    params.hdf5_version = version;

    const char* exemplar = "2023-11-02T23:01:02Z";
    std::vector<const char*> dump(params.num_rows, exemplar);
    H5::StrType stype(0, H5T_VARIABLE);

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup(name);
        create_hdf5_data_frame(ghandle, params.num_rows, false, columns, version);
        auto dhandle = ghandle.openGroup("data");
        dhandle.unlink("0");
        auto xhandle = spawn_string_column(dhandle, "0", params.num_rows, version, stype);
        if (version >= 3) {
            attach_format(xhandle, "date-time");
        }
        xhandle.write(dump.data(), stype);
    }
    takane::hdf5_data_frame::validate(path.c_str(), params);

    if (version >= 3) {
        {
            H5::H5File handle(path, H5F_ACC_RDWR);
            auto ghandle = handle.openGroup(name);
            auto dhandle = ghandle.openGroup("data");
            dhandle.unlink("0");
            auto xhandle = spawn_string_column(dhandle, "0", params.num_rows, version, stype);
            attach_format(xhandle, "none");
            xhandle.write(dump.data(), stype);
        }
        expect_error("'format' attribute set to 'date-time'", path.c_str(), params);
    }

    const char* violator = "asdasd";
    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        auto ghandle = handle.openGroup(name);
        auto dhandle = ghandle.openGroup("data");
        dhandle.unlink("0");
        auto xhandle = spawn_string_column(dhandle, "0", params.num_rows, version, stype);
        if (version >= 3) {
            attach_format(xhandle, "date-time");
        }
        dump.back() = violator;
        xhandle.write(dump.data(), stype);
    }
    expect_error("date/time-formatted", path.c_str(), params);

    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        auto ghandle = handle.openGroup(name);
        auto dhandle = ghandle.openGroup("data");
        auto xhandle = dhandle.openDataSet("0");
        auto ahandle = xhandle.createAttribute("missing-value-placeholder", stype, H5S_SCALAR);
        ahandle.write(stype, std::string(violator));
    }
    takane::hdf5_data_frame::validate(path.c_str(), params);
}

TEST_P(Hdf5DataFrameTest, Factor) {
    takane::hdf5_data_frame::Parameters params(name);
    params.num_rows = 32;
    auto& columns = params.columns.mutable_ref();
    columns.resize(1);
    columns[0].name = "Aaron";
    columns[0].type = takane::data_frame::ColumnType::FACTOR;
    std::vector<std::string> levels{ "kanon", "chisato", "sumire", "ren", "keke" };
    columns[0].factor_levels.mutable_ref().insert(levels.begin(), levels.end());

    auto version = GetParam();
    params.df_version = version;
    params.hdf5_version = version;

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup(name);
        create_hdf5_data_frame(ghandle, params.num_rows, false, columns, version);
    }
    takane::hdf5_data_frame::validate(path.c_str(), params);

    if (version == 1) {
        columns[0].factor_levels.mutable_ref().erase("chisato");
        expect_error("contains 'chisato'", path.c_str(), params);

        hsize_t nrows = params.num_rows;
        H5::DataSpace dspace(1, &nrows);
        {
            H5::H5File handle(path, H5F_ACC_RDWR);
            auto ghandle = handle.openGroup(name);
            auto dhandle = ghandle.openGroup("data");
            dhandle.unlink("0");
            dhandle.createDataSet("0", H5::PredType::NATIVE_DOUBLE, dspace);
        }
        expect_error("string dataset", path.c_str(), params);

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
        takane::hdf5_data_frame::validate(path.c_str(), params);

    } else {
        std::string code_name, group_name;
        if (version == 2) {
            group_name = "data";
            code_name = "0";
        } else {
            group_name = "data/0";
            code_name = "codes";
        }

        {
            H5::H5File handle(path, H5F_ACC_RDWR);
            auto ghandle = handle.openGroup(name);
            auto dhandle = ghandle.openGroup(group_name);
            dhandle.unlink(code_name);
            hsize_t nrows = params.num_rows + 10;
            H5::DataSpace dspace(1, &nrows);
            dhandle.createDataSet(code_name, H5::PredType::NATIVE_INT8, dspace);
        }
        expect_error("length equal to the number of rows", path.c_str(), params);

        hsize_t nrows = params.num_rows;
        H5::DataSpace dspace(1, &nrows);
        {
            H5::H5File handle(path, H5F_ACC_RDWR);
            auto ghandle = handle.openGroup(name);
            auto dhandle = ghandle.openGroup(group_name);
            dhandle.unlink(code_name);
            dhandle.createDataSet(code_name, H5::PredType::NATIVE_DOUBLE, dspace);
        }
        expect_error("expected factor column", path.c_str(), params);

        {
            H5::H5File handle(path, H5F_ACC_RDWR);
            auto ghandle = handle.openGroup(name);
            auto dhandle = ghandle.openGroup(group_name);
            dhandle.unlink(code_name);
            dhandle.createDataSet(code_name, H5::PredType::NATIVE_INT64, dspace);
        }
        expect_error("32-bit signed integer", path.c_str(), params);

        {
            H5::H5File handle(path, H5F_ACC_RDWR);
            auto ghandle = handle.openGroup(name);
            auto dhandle = ghandle.openGroup(group_name);
            dhandle.unlink(code_name);

            auto xhandle = dhandle.createDataSet(code_name, H5::PredType::NATIVE_INT16, dspace);
            std::vector<int> replacement(nrows, columns[0].factor_levels->size());
            xhandle.write(replacement.data(), H5::PredType::NATIVE_INT);
        }
        expect_error("less than the number of levels", path.c_str(), params);

        // Using -1 as a placeholder value.
        {
            H5::H5File handle(path, H5F_ACC_RDWR);
            auto ghandle = handle.openGroup(name);
            auto dhandle = ghandle.openGroup(group_name);
            dhandle.unlink(code_name);

            auto xhandle = dhandle.createDataSet(code_name, H5::PredType::NATIVE_INT16, dspace);
            std::vector<int> replacement(nrows, -1);
            xhandle.write(replacement.data(), H5::PredType::NATIVE_INT);
        }
        expect_error("non-negative", path.c_str(), params);

        {
            H5::H5File handle(path, H5F_ACC_RDWR);
            auto ghandle = handle.openGroup(name);
            auto dhandle = ghandle.openGroup(group_name);
            auto xhandle = dhandle.openDataSet(code_name);
            auto ahandle = xhandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_INT16, H5S_SCALAR);
            int val = -1;
            ahandle.write(H5::PredType::NATIVE_INT, &val); 
        }
        takane::hdf5_data_frame::validate(path.c_str(), params); // rescues the negative values.
    }

    if (version >= 3) {
        {
            H5::H5File handle(path, H5F_ACC_TRUNC);
            auto ghandle = handle.createGroup(name);
            create_hdf5_data_frame(ghandle, params.num_rows, false, columns, version);
            auto dhandle = ghandle.openGroup("data/0");
            dhandle.unlink("levels");
            dhandle.createDataSet("levels", H5::PredType::NATIVE_INT, H5S_SCALAR);
        }
        expect_error("string datatype", path.c_str(), params);

        {
            H5::H5File handle(path, H5F_ACC_RDWR);
            auto ghandle = handle.openGroup(name);
            auto dhandle = ghandle.openGroup("data/0");
            dhandle.unlink("levels");

            std::vector<std::string> levels(columns[0].factor_levels->begin(), columns[0].factor_levels->end());
            levels.push_back(levels[0]);
            auto dump = pointerize_strings(levels);

            hsize_t nlevels = dump.size();
            H5::DataSpace dspace(1, &nlevels);
            H5::StrType stype(0, H5T_VARIABLE);
            auto xhandle = dhandle.createDataSet("levels", stype, dspace);
            xhandle.write(dump.data(), stype);
        }
        expect_error("duplicate level", path.c_str(), params);

        {
            H5::H5File handle(path, H5F_ACC_TRUNC);
            auto ghandle = handle.createGroup(name);
            create_hdf5_data_frame(ghandle, params.num_rows, false, columns, version);
            auto dhandle = ghandle.openGroup("data/0");
            dhandle.removeAttr("type");
            attach_type(dhandle, "WHEE");
        }
        expect_error("'type' attribute set to 'factor'", path.c_str(), params);

        {
            H5::H5File handle(path, H5F_ACC_RDWR);
            auto ghandle = handle.openGroup(name);
            auto dhandle = ghandle.openGroup("data/0");
            dhandle.removeAttr("type");
            attach_type(dhandle, "factor");
            dhandle.createAttribute("ordered", H5::PredType::NATIVE_UINT8, H5S_SCALAR);
        }
        takane::hdf5_data_frame::validate(path.c_str(), params);
        auto params2 = params;
        params2.columns.mutable_ref()[0].factor_ordered = true;
        expect_error("not consistent", path.c_str(), params2);

        {
            H5::H5File handle(path, H5F_ACC_RDWR);
            auto ghandle = handle.openGroup(name);
            auto dhandle = ghandle.openGroup("data/0");
            dhandle.removeAttr("ordered");
            dhandle.createAttribute("ordered", H5::PredType::NATIVE_DOUBLE, H5S_SCALAR);
        }
        expect_error("32-bit signed integer", path.c_str(), params);
    }
}

INSTANTIATE_TEST_SUITE_P(
    Hdf5DataFrame,
    Hdf5DataFrameTest,
    ::testing::Values(1,2,3) // versions
);
