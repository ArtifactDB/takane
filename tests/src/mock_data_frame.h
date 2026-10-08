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
    VLS,
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
    const hsize_t NC = columns.size();
    add_hdf5_numeric_attribute(handle, "row-count", H5::PredType::NATIVE_UINT32, num_rows);

    {
        std::vector<const char*> column_names;
        column_names.reserve(NC);
        for (const auto& col : columns) {
            column_names.push_back(col.name.c_str());
        }
        H5::StrType stype(0, H5T_VARIABLE);
        auto dhandle = handle.createDataSet("column_names", stype, H5::DataSpace(1, &NC));
        dhandle.write(column_names.data(), stype);
    }

    auto ghandle = handle.createGroup("data");
    for (hsize_t c = 0; c < NC; ++c) {
        const auto& curcol = columns[c];
        if (curcol.type == DataFrameColumnType::OTHER) {
            continue;
        }

        std::string colname = std::to_string(c);
        if (curcol.type == DataFrameColumnType::INTEGER) {
            auto dhandle = ghandle.createDataSet(colname, H5::PredType::NATIVE_INT32, create_hdf5_dataspace(num_rows));
            add_hdf5_string_attribute(dhandle, "type", "integer");

        } else if (curcol.type == DataFrameColumnType::NUMBER) {
            auto dhandle = ghandle.createDataSet(colname, H5::PredType::NATIVE_DOUBLE, create_hdf5_dataspace(num_rows));
            add_hdf5_string_attribute(dhandle, "type", "number");

        } else if (curcol.type == DataFrameColumnType::BOOLEAN) {
            auto dhandle = ghandle.createDataSet(colname, H5::PredType::NATIVE_INT8, create_hdf5_dataspace(num_rows));
            add_hdf5_string_attribute(dhandle, "type", "boolean");

        } else if (curcol.type == DataFrameColumnType::STRING) {
            auto dhandle = ghandle.createDataSet(colname, H5::StrType(0, curcol.string_length), create_hdf5_dataspace(num_rows));
            add_hdf5_string_attribute(dhandle, "type", "string");

        } else if (curcol.type == DataFrameColumnType::FACTOR) {
            auto dhandle = ghandle.createGroup(colname);
            add_hdf5_string_attribute(dhandle, "type", "factor");
            if (curcol.factor_ordered) {
                add_hdf5_numeric_attribute(dhandle, "ordered", H5::PredType::NATIVE_INT8, 1);
            }

            const hsize_t nchoices = curcol.factor_levels.size();
            add_hdf5_string_dataset(dhandle, "levels", curcol.factor_levels);

            // Just make up whatever here.
            std::vector<int> codes(num_rows);
            for (hsize_t i = 0; i < num_rows; ++i) {
                codes[i] = i % nchoices;
            }
            add_hdf5_numeric_dataset(dhandle, "codes", H5::PredType::NATIVE_UINT16, codes);

        } else if (curcol.type == DataFrameColumnType::VLS) {
            auto vhandle = ghandle.createGroup(colname);
            add_hdf5_string_attribute(vhandle, "type", "vls");
            auto vtype = ritsuko::cvls::define_pointer_datatype<std::uint64_t, std::uint64_t>();
            auto phandle = vhandle.createDataSet("pointers", vtype, H5::DataSpace(1, &num_rows));

            std::vector<ritsuko::cvls::Pointer<std::uint64_t, std::uint64_t> > buffer(num_rows);
            hsize_t previous = 0;
            for (hsize_t i = 0; i < num_rows; ++i) {
                buffer[i].offset = previous; 
                const auto len = 1 + i % 10;
                buffer[i].length = len;
                previous += len;
            }
            phandle.write(buffer.data(), vtype);

            vhandle.createDataSet("heap", H5::PredType::NATIVE_UINT8, create_hdf5_dataspace(previous));
        }
    }

    return handle;
}

inline H5::Group mock_data_frame(const std::filesystem::path& path, hsize_t num_rows, const std::vector<DataFrameColumnDetails>& columns) {
    std::string vstring = "1.0";
    for (const auto& col : columns) {
        if (col.type == DataFrameColumnType::VLS) {
            vstring = "1.1";
            break;
        }
    }

    initialize_directory_simple(path, "data_frame", vstring);
    H5::H5File handle(path / "basic_columns.h5", H5F_ACC_TRUNC);
    auto ghandle = handle.createGroup("data_frame");
    return mock_data_frame(ghandle, num_rows, columns);
}

inline void attach_row_names_to_data_frame(H5::Group& handle, hsize_t num_rows) {
    H5::DataSpace dspace(1, &num_rows);
    H5::StrType stype(0, H5T_VARIABLE);
    auto dhandle = handle.createDataSet("row_names", stype, dspace);
    std::vector<std::string> row_names(num_rows);
    for (hsize_t i = 0; i < num_rows; ++i) {
        row_names[i] = std::to_string(i);
    }
    auto row_names_ptr = pointerize_strings(row_names);
    dhandle.write(row_names_ptr.data(), stype);
}

#endif
