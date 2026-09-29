#ifndef MOCK_DATA_FRAME_H
#define MOCK_DATA_FRAME_H

#include <vector>
#include <string>
#include <numeric>

#include "H5Cpp.h"
#include "utils.h"

enum class DataFrameColumnType {
    INTEGER,
    NUMBER,
    STRING,
    BOOLEAN,
    FACTOR,
    OTHER
};

struct DataFrameColumnDetails {
    std::string name;
    DataFrameColumnType type = DataFrameColumnType::INTEGER;
    std::size_t string_length = 11;
    bool factor_ordered = false;
    std::vector<std::string> factor_levels;
};

inline H5::Group mock_data_frame(H5::Group& handle, hsize_t num_rows, const std::vector<DataFrameColumnDetails>& columns) {
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

    auto attr = handle.createAttribute("row-count", H5::PredType::NATIVE_UINT32, H5S_SCALAR);
    attr.write(H5::PredType::NATIVE_HSIZE, &num_rows);

    auto ghandle = handle.createGroup("data");
    const std::size_t NC = columns.size();
    for (std::size_t c = 0; c < NC; ++c) {
        const auto& curcol = columns[c];
        if (curcol.type == DataFrameColumnType::OTHER) {
            continue;
        }

        std::string colname = std::to_string(c);
        if (curcol.type == DataFrameColumnType::INTEGER) {
            auto dhandle = add_hdf5_dataset(ghandle, colname, H5::PredType::NATIVE_INT32, num_rows);
            add_hdf5_attribute(dhandle, "type", "integer");

        } else if (curcol.type == DataFrameColumnType::NUMBER) {
            auto dhandle = add_hdf5_dataset(ghandle, colname, H5::PredType::NATIVE_DOUBLE, num_rows);
            add_hdf5_attribute(dhandle, "type", "number");

        } else if (curcol.type == DataFrameColumnType::BOOLEAN) {
            auto dhandle = add_hdf5_dataset(ghandle, colname, H5::PredType::NATIVE_INT8, num_rows);
            add_hdf5_attribute(dhandle, "type", "boolean");

        } else if (curcol.type == DataFrameColumnType::STRING) {
            auto dhandle = add_hdf5_dataset(ghandle, colname, H5::StrType(0, curcol.string_length), num_rows);
            add_hdf5_attribute(dhandle, "type", "string");

        } else if (curcol.type == DataFrameColumnType::FACTOR) {
            auto dhandle = ghandle.createGroup(colname);
            add_hdf5_attribute(dhandle, "type", "factor");
            if (curcol.factor_ordered) {
                auto ahandle = dhandle.createAttribute("ordered", H5::PredType::NATIVE_INT8, H5S_SCALAR);
                constexpr int val = 1;
                ahandle.write(H5::PredType::NATIVE_INT, &val);
            }

            hsize_t nchoices = curcol.factor_levels.size();
            auto lhandle = add_hdf5_dataset(dhandle, "levels", H5::StrType(0, H5T_VARIABLE), nchoices);
            std::vector<const char*> ptrs;
            for (const auto& lev : curcol.factor_levels) {
                ptrs.push_back(lev.c_str());
            }
            lhandle.write(ptrs.data(), H5::StrType(0, H5T_VARIABLE));

            std::vector<int> codes(num_rows);
            for (hsize_t i = 0; i < num_rows; ++i) {
                codes[i] = i % nchoices;
            }
            auto chandle = add_hdf5_dataset(dhandle, "codes", H5::PredType::NATIVE_UINT16, num_rows);
            chandle.write(codes.data(), H5::PredType::NATIVE_INT);
        }
    }

    return handle;
}

inline H5::Group mock_data_frame(const std::filesystem::path& path, hsize_t num_rows, const std::vector<DataFrameColumnDetails>& columns) {
    initialize_directory_simple(path, "data_frame", "1.0");
    H5::H5File handle(path / "basic_columns.h5", H5F_ACC_TRUNC);
    auto ghandle = handle.createGroup("data_frame");
    return mock_data_frame(ghandle, num_rows, columns);
}

inline void attach_row_names_to_data_frame(H5::Group& handle, hsize_t num_rows) {
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

#endif
