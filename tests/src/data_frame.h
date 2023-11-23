#ifndef DATA_FRAME_H
#define DATA_FRAME_H

#include <vector>
#include <string>
#include <numeric>

#include "H5Cpp.h"
#include "utils.h"

namespace data_frame {

enum class ColumnType {
    INTEGER,
    NUMBER,
    STRING,
    BOOLEAN,
    FACTOR,
    OTHER
};

struct ColumnDetails {
    std::string name;

    ColumnType type = ColumnType::INTEGER;

    bool factor_ordered = false;

    std::vector<std::string> factor_levels;
};

inline void mock(H5::Group& handle, hsize_t num_rows, const std::vector<data_frame::ColumnDetails>& columns) {
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

    H5::StrType stype(0, H5T_VARIABLE);
    auto attr2 = handle.createAttribute("version", stype, H5S_SCALAR);
    attr2.write(stype, std::string("1.0"));

    auto ghandle = handle.createGroup("data");
    size_t NC = columns.size();
    for (size_t c = 0; c < NC; ++c) {
        const auto& curcol = columns[c];
        if (curcol.type == data_frame::ColumnType::OTHER) {
            continue;
        }

        std::string colname = std::to_string(c);
        if (curcol.type == data_frame::ColumnType::INTEGER) {
            auto dhandle = hdf5_utils::spawn_data(ghandle, colname, num_rows, H5::PredType::NATIVE_INT32);
            std::vector<int> dump(num_rows);
            std::iota(dump.begin(), dump.end(), 0);
            dhandle.write(dump.data(), H5::PredType::NATIVE_INT);
            hdf5_utils::attach_attribute(dhandle, "type", "integer");

        } else if (curcol.type == data_frame::ColumnType::NUMBER) {
            std::vector<double> dump(num_rows);
            std::iota(dump.begin(), dump.end(), 0.5);
            auto dhandle = hdf5_utils::spawn_data(ghandle, colname, num_rows, H5::PredType::NATIVE_DOUBLE);
            dhandle.write(dump.data(), H5::PredType::NATIVE_DOUBLE);
            hdf5_utils::attach_attribute(dhandle, "type", "number");

        } else if (curcol.type == data_frame::ColumnType::BOOLEAN) {
            std::vector<int> dump(num_rows);
            for (hsize_t i = 0; i < num_rows; ++i) {
                dump[i] = i % 2;
            }
            auto dhandle = hdf5_utils::spawn_data(ghandle, colname, num_rows, H5::PredType::NATIVE_INT8);
            dhandle.write(dump.data(), H5::PredType::NATIVE_INT);
            hdf5_utils::attach_attribute(dhandle, "type", "boolean");

        } else if (curcol.type == data_frame::ColumnType::STRING) {
            std::vector<std::string> raw_dump(num_rows);
            for (hsize_t i = 0; i < num_rows; ++i) {
                raw_dump[i] = std::to_string(i);
            }
            auto dhandle = hdf5_utils::spawn_string_data(ghandle, colname, H5T_VARIABLE, raw_dump);
            hdf5_utils::attach_attribute(dhandle, "type", "string");

        } else if (curcol.type == data_frame::ColumnType::FACTOR) {
            auto dhandle = ghandle.createGroup(colname);
            hdf5_utils::attach_attribute(dhandle, "type", "factor");
            if (curcol.factor_ordered) {
                hdf5_utils::attach_attribute(dhandle, "ordered", 1);
            }

            hsize_t nchoices = curcol.factor_levels.size();
            hdf5_utils::spawn_string_data(dhandle, "levels", H5T_VARIABLE, curcol.factor_levels);

            std::vector<int> codes(num_rows);
            for (hsize_t i = 0; i < num_rows; ++i) {
                codes[i] = i % nchoices;
            }
            auto chandle = hdf5_utils::spawn_data(dhandle, "codes", num_rows, H5::PredType::NATIVE_INT16);
            chandle.write(codes.data(), H5::PredType::NATIVE_INT);
        }
    }
}

inline void mock(const std::filesystem::path& path, hsize_t num_rows, const std::vector<data_frame::ColumnDetails>& columns) {
    initialize_directory(path, "data_frame");
    H5::H5File handle(path / "basic_columns.h5", H5F_ACC_TRUNC);
    auto ghandle = handle.createGroup("data_frame");
    mock(ghandle, num_rows, columns);
}

inline void attach_row_names(H5::Group& handle, hsize_t num_rows) {
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

}

#endif
