#ifndef TAKANE_HDF5_DATA_FRAME_HPP
#define TAKANE_HDF5_DATA_FRAME_HPP

#include "H5Cpp.h"
#include "ritsuko/hdf5/hdf5.hpp"

#include <cstdint>
#include <vector>
#include <string>

namespace takane {

namespace hdf5_data_frame {

inline void validate(const std::string& file, const std::string& name, int nrow, bool has_row_names, const std::vector<ColumnDetails>& columns, int version = 2) {
    H5::H5File handle(file);
    auto ghandle = handle.openGroup(name);
    constexpr hsize_t buffer_size = 10000;
    const char* missing_attr = "missing-value-placeholder";

    // Checking the row names.
    if (has_row_names) {
        if (!ghandle.exists("row_names") || ghandle.childObjType("row_names") != H5O_DATASET) {
            throw std::runtime_error("expected a 'row_names' dataset when row names is present");
        }

        auto rnhandle = ghandle.openDataSet("row_names");
        if (rnhandle.getTypeClass() != H5T_STRING) {
            throw std::runtime_error("expected 'row_names' to be a string dataset");
        }

        if (ritsuko::hdf5::get_1d_length(rnhandle.getSpace(), "row_names") != nrow) {
            throw std::runtime_error("expected 'row_names' to have length equal to the number of rows");
        }
    }

    // Checking the column names.
    {
        if (!ghandle.exists("column_names") || ghandle.childObjType("column_names") != H5T_DATASET) {
            throw std::runtime_error("expected a 'column_names' dataset");
        }

        auto cnhandle = ghandle.openDataSet("column_names");
        if (cnhandle.getTypeClass() != H5T_STRING) {
            throw std::runtime_error("expected 'column_names' to be a string dataset");
        }

        size_t ncols = ritsuko::hdf5::get_1d_length(cnhandle.getSpace(), "column_names");
        if (ncols != columns.size()) {
            throw std::runtime_error("length of 'column_names' should equal the expected number of columns");
        }

        ritsuko::hdf5::load_1d_string_dataset(
            cnhandle, 
            ncols, 
            buffer_size,
            [&](size_t i, const char* p, size_t l) {
                const auto& expected = columns[i].name;
                if (l != expected.size() || strncmp(expected.c_str(), p, l)) {
                    throw std::runtime_error("expected column name '" + expected + "' but got '" + std::string(p, p + l) + "' for column " + std::to_string(i));
                }
            }
        );
    }

    // Checking each column individually.
    size_t NC = columns.size();
    for (size_t c = 0; c < NC; ++c) {
        const auto& curcol = columns[c];

        std::string dset_name = std::to_string(c);
        if (!ghandle.exists(dset_name)) {
            if (curcol.type == data_frame::ColumnType::OTHER) {
                continue;
            }
        }

        if (ghandle.childObjType(dset_name) != H5T_DATASET) {
            throw std::runtime_error("expected a HDF5 dataset for column " + dset_name);
        }

        auto dhandle = ghandle.openDataSet(dset_name);
        if (nrow != ritsuko::hdf5::get_1d_length(dhandle, dset_name.c_str())) {
            throw std::runtime_error("expected column " + dset_name + " to have length equal to the number of rows");
        }

        if (curcol.type == data_frame::ColumnType::NUMBER) {
            if (dhandle.getTypeClass() != H5T_FLOAT) {
                throw std::runtime_error("expected column " + dset_name + " to be a floating-point dataset");
            }
            if (version > 1 && dhandle.attrExists(missing_attr)) {
                ritsuko::hdf5::get_missing_placeholder_attribute(dhandle, missing_attr, dset_name);
            }

        } else if (curcol.type == data_frame::ColumnType::BOOLEAN || curcol.type == data_frame::ColumnType::INTEGER || curcol.type == data_frame::ColumnType::FACTOR) {
            if (dhandle.getTypeClass() != H5T_INTEGER) {
                throw std::runtime_error("expected column " + dset_name + " to be an integer dataset");
            }
            ritsuko::hdf5::forbid_large_integers(dhandle, 32, dset_name.c_str());

            bool has_missing = false;
            int32_t placeholder = -2147483648;
            if (version == 1){
                has_missing = true;
            } else if (version > 1) {
                if (dhandle.attrExists(missing_attr)) {
                    auto attr = ritsuko::hdf5::get_missing_placeholder_attribute(dhandle, missing_attr, dset_name);
                    attr.read(&placeholder, H5::PredType::NATIVE_INT32);
                }
            }

            if (curcol.type == data_frame::ColumnType::FACTOR) {
                auto block_size = ritsuko::hdf5::pick_1d_block_size(dhandle.getCreatePlist(), nrow, buffer_size);
                std::vector<int32_t> buffer(block_size);
                ritsuko::hdf5::iterate_1d_blocks(block_size, nrow,
                    [&](hsize_t start, hsize_t len, const H5::DataSpace& memspace, const H5::DataSpace& dataspace) {
                        dhandle.read(buffer.data(), H5::Predtype::NATIVE_INT32, memspace, dataspace);
                        for (hsize_t i = 0; i < len; ++i) {
                            if (has_missing && buffer[i] == placeholder) {
                                continue;
                            }
                            if (buffer[i] < 0) {
                                throw std::runtime_error("expected factor indices to be positive in column " + dset_name);
                            }
                            if (buffer[i] >= curcol.factor_levels) {
                                throw std::runtime_error("expected factor indices to less than the number of levels in column " + dset_name);
                            }
                        }
                    }
                );
            }

        } else if (curcol.type == data_frame::ColumnType::STRING) {
            if (dhandle.getTypeClass() != H5T_STRING) {
                throw std::runtime_error("expected column " + dset_name + " to be a string dataset");
            }
            if (version > 1 && dhandle.attrExists(missing_attr)) {
                ritsuko::hdf5::get_missing_placeholder_attribute<true>(dhandle, missing_attr, dset_name);
            }

        } else {
            throw std::runtime_error("expected no dataset for 'other' column " + dset_name);
        }
    }
}

}

}

#endif
