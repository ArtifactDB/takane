#ifndef TAKANE_HDF5_FRAME_HPP
#define TAKANE_HDF5_FRAME_HPP

#include "H5Cpp.h"
#include "ritsuko/ritsuko.hpp"
#include "ritsuko/hdf5/hdf5.hpp"
#include "ritsuko/hdf5/vls/vls.hpp"

#include <cstdint>
#include <cstddef>
#include <string>
#include <stdexcept>
#include <vector>
#include <filesystem>
#include <unordered_set>

#include "utils_public.hpp"
#include "utils_string.hpp"
#include "utils_factor.hpp"
#include "utils_other.hpp"
#include "utils_json.hpp"
#include "utils_missing.hpp"

/**
 * @file data_frame.hpp
 * @brief Validation for data frames.
 */

namespace takane {

/**
 * @cond
 */
void validate(const std::filesystem::path&, const ObjectMetadata&, const Options& options);
std::size_t height(const std::filesystem::path&, const ObjectMetadata&, const Options& options);

inline hsize_t validate_column(const H5::Group& dhandle, const std::string& dset_name, const ritsuko::Version& version, const Options& options) { 
    const char* missing_attr_name = "missing-value-placeholder";

    auto dtype = dhandle.childObjType(dset_name);
    hsize_t output;

    if (dtype == H5O_TYPE_GROUP) {
        auto ghandle = dhandle.openGroup(dset_name);
        auto type = open_and_load_scalar_string_attribute(ghandle, "type");
n
        if (type == "factor") {
            check_factor_ordered_attribute(ghandle);
            auto num_levels = validate_factor_levels(ghandle, "levels", options.hdf5_buffer_size);
            output = validate_factor_codes(ghandle, "codes", num_levels, options.hdf5_buffer_size, /* allow_missing = */ true);

        } else if (type == "vls") {
            if (version.lt(1, 1, 0)) {
                throw std::runtime_error("unsupported type '" + type + "'");
            }

            auto hhandle = ghandle.openDataSet("heap");
            auto hlen = ritsuko::cvls::validate_heap(hhandle);

            auto phandle = ghandle.openDataSet("pointers");
            auto pspace = phandle.getSpace();
            if (pspace.getSimpleExtentNdims() != 1) {
                throw std::runtime_error("expected 'pointers' to be a 1-dimensional dataset");
            }
            pspace.getSimpleExtentDims(&output);

            ritsuko::cvls::validate_1d_pointers<std::uint64_t, std::uint64_t>(
                phandle,
                output,
                hlen,
                [&]{
                    ritsuko::cvls::Validate1dPointersOptions opt;
                    opt.contiguous_chunk_size = options.hdf5_buffer_size;
                    return opt;
                }()
            );

            check_string_missing_placeholder_attribute(attr, missing_attr_name);

        } else {
            throw std::runtime_error("unsupported type '" + type + "'");
        }

    } else if (dtype == H5O_TYPE_DATASET) {
        auto xhandle = dhandle.openDataSet(dset_name);
        auto xspace = xhandle.getSpace();
        if (xspace.getSimpleExtentNdims() != 1) {
            throw std::runtime_error("expected column to be a 1-dimensional dataset");
        }
        xspace.getSimpleExtentDims(&output);

        auto type = open_and_load_scalar_string_attribute(xhandle, "type");
        if (type == "string") {
            if (!ritsuko::hdf5::is_utf8_string(xhandle)) {
                throw std::runtime_error("expected a datatype that can be represented by a UTF-8 encoded string");
            }
            auto missingness = read_string_missing_placeholder(xhandle, missing_attr_name);
            std::string format = open_and_load_format_attribute(xhandle);
            validate_string_format(xhandle, output, format, missingness, options.hdf5_buffer_size);

        } else {
            if (type == "integer") {
                if (ritsuko::hdf5::exceeds_integer_limit(xhandle, 32, true)) {
                    throw std::runtime_error("expected integer column to use a datatype that is a subset of a 32-bit signed integer");
                }
            } else if (type == "boolean") {
                if (ritsuko::hdf5::exceeds_integer_limit(xhandle, 32, true)) {
                    throw std::runtime_error("expected boolean column to use a datatype that is a subset of a 32-bit signed integer");
                }
            } else if (type == "number") {
                if (ritsuko::hdf5::exceeds_float_limit(xhandle, 64)) {
                    throw std::runtime_error("expected number column to use a datatype that is a subset of a 64-bit float");
                }
            } else {
                throw std::runtime_error("unknown column type '" + type + "'");
            }

            check_numeric_missing_placeholder(xhandle, missing_attr_name);
        }

    } else {
        throw std::runtime_error("unknown HDF5 object type");
    }

    return output;
}
/**
 * @endcond
 */

/**
 * @param path Path to the directory containing the data frame.
 * @param metadata Metadata for the object, typically read from its `OBJECT` file.
 * @param options Validation options.
 */
inline void validate_data_frame(const std::filesystem::path& path, const ObjectMetadata& metadata, Options& options) {
    const std::string type_name = "data_frame"; // use a separate variable to avoid dangling reference warnings from GCC.

    const auto& type_meta = extract_json_type_metadata(metadata.other, type_name);
    const auto& vstring = extract_json_version_string(type_meta, type_name);
    auto version = ritsuko::parse_version_string(vstring.c_str(), vstring.size(), /* skip_patch = */ true);
    if (version.major != 1) {
        throw std::runtime_error("unsupported version '" + vstring + "'");
    }

    H5::H5File handle(path / "basic_columns.h5", H5F_ACC_RDONLY);
    auto ghandle = handle.openGroup(type_name);

    // Checking the number of rows.
    auto attr = ghandle.openAttribute("row-count");
    if (attr.getSpace().getSimpleExtentNdims() != 0) {
        throw std::runtime_error("'row-count' attribute should have a scalar");
    }
    if (ritsuko::hdf5::exceeds_integer_limit(attr, 64, false)) {
        throw std::runtime_error("'row-count' attribute should have a datatype that fits in a 64-bit unsigned integer");
    }
    std::uint64_t num_rows = 0;
    attr.read(H5::PredType::NATIVE_UINT64, &num_rows);

    // Checking row and column names.
    if (ghandle.exists("row_names")) {
        auto rnhandle = handle.openDataSet("row_names");
        if (!ritsuko::hdf5::is_utf8_string(rnhandle)) {
            throw std::runtime_error("expected a datatype for 'row_names' that can be represented by a UTF-8 encoded string");
        }

        auto rnspace = rnhandle.getSpace();
        if (rnspace.getSimpleExtentNdims() != 1) {
            throw std::runtime_error("expected 'row_names' to be a 1-dimensional dataset");
        }
        hsize_t num_names;
        rnspace.getSimpleExtentDims(&num_names);
        if (!sanisizer::is_equal(num_names, num_rows)) {
            throw std::runtime_error("expected 'row_names' to have length equal to the number of rows");
        }

        try {
            ritsuko::hdf5::validate_1d_string_dataset(
                rnhandle,
                num_rows,
                [&]{
                    ritsuko::hdf5::Validate1dStringDatasetOptions opt;
                    opt.contiguous_chunk_size = options.hdf5_buffer_size;
                    return opt;
                }()
            );
        } catch (std::exception& e) {
            throw std::runtime_error("failed to validate row names; " + std::string(e.what()));
        }
    }

    hsize_t NC;
    {
        auto cnhandle = ghandle.openDataSet("column_names");
        if (!ritsuko::hdf5::is_utf8_string(cnhandle)) {
            throw std::runtime_error("expected a datatype for 'column_names' that can be represented by a UTF-8 encoded string");
        }

        auto cnspace = cnhandle.getSpace();
        if (cnspace.getSimpleExtentNdims() != 1) {
            throw std::runtime_error("expected 'column_names' to be a 1-dimensional dataset");
        }
        cnspace.getSimpleExtentDims(&NC);        

        ritsuko::hdf5::Stream1dStringDataset stream(
            &cnhandle,
            NC,
            [&]{
                ritsuko::hdf5::Stream1dStringDatasetOptions opt;
                opt.contiguous_chunk_size = options.hdf5_buffer_size;
                return opt;
            }()
        );

        std::unordered_set<std::string> column_names;
        iterate_stream(
            stream,
            [&](hsize_t, std::string x) -> void {
                if (x.empty()) {
                    throw std::runtime_error("column names should not be empty strings");
                }
                if (column_names.find(x) != column_names.end()) {
                    throw std::runtime_error("duplicated column name '" + x + "'");
                }
                column_names.insert(std::move(x));
            }
        );
    }

    // Finally iterating through the columns.
    auto dhandle = ghandle.openGroup("data");
    hsize_t num_basic = 0;
    auto other_dir = path / "other_columns";

    for (I<decltype(NC)> c = 0; c < NC; ++c) {
        std::string dset_name = std::to_string(c);

        if (!dhandle.exists(dset_name)) {
            auto opath = other_dir / dset_name;
            auto ometa = read_object_metadata(opath);
            try {
                ::takane::validate(opath, ometa, options);
            } catch (std::exception& e) {
                throw std::runtime_error("failed to validate 'other' column " + dset_name + "; " + std::string(e.what()));
            }
            if (::takane::height(opath, ometa, options) != num_rows) {
                throw std::runtime_error("height of column " + dset_name + " of class '" + ometa.type + "' is not the same as the number of rows");
            }

        } else {
            hsize_t colsize;
            try {
                colsize = validate_column(dhandle, dset_name, num_rows, options);
            } catch (std::exception& e) {
                throw std::runtime_error("failed to validate column " + dset_name + "'; " + std::string(e.what()));
            }
            if (!sanisizer::is_equal(colsize, num_rows)) {
                throw std::runtime_error("length of column " + dset_name + " is not equal to the number of rows");
            }
            ++num_basic;
        }
    }

    if (std::filesystem::exists(other_dir)) {
        if (internal_other::count_directory_entries(other_dir) != NC - num_basic) {
            throw std::runtime_error("more objects than expected inside the 'other_columns' directory");
        }
    }

    if (num_basic != dhandle.getNumObjs()) {
        throw std::runtime_error("more objects present in the 'data_frame/data' group than expected");
    }

    validate_mcols(path, "column_annotations", sanisizer::cast<std::size_t>(NC), options);
    validate_metadata(path, "other_annotations", options);
}

/**
 * @param path Path to a directory containing a data frame.
 * @param metadata Metadata for the object, typically read from its `OBJECT` file.
 * @param options Validation options.
 * @return The number of rows.
 */
inline std::size_t height_of_data_frame(const std::filesystem::path& path, [[maybe_unused]] const ObjectMetadata& metadata, [[maybe_unused]] const Options& options) {
    H5::H5File handle(path / "basic_columns.h5", H5F_ACC_RDONLY);
    auto ghandle = handle.openGroup("data_frame");
    auto ahandle = ghandle.openAttribute("row-count");
    std::uint64_t output;
    ahandle.read(&output, H5::PredType::NATIVE_UINT64);
    return sanisizer::cast<std::size_t>(output);
}

/**
 * @param path Path to a directory containing a data frame.
 * @param metadata Metadata for the object, typically read from its `OBJECT` file.
 * @param options Validation options.
 * @return A vector of length 2 containing the number of rows and columns in the data frame.
 */
inline std::vector<size_t> dimensions_of_data_frame(const std::filesystem::path& path, [[maybe_unused]] const ObjectMetadata& metadata, [[maybe_unused]] const Options& options) {
    std::vector<size_t> output(2);

    H5::H5File handle(path / "basic_columns.h5", H5F_ACC_RDONLY);
    auto ghandle = handle.openGroup("data_frame");

    auto ahandle = ghandle.openAttribute("row-count");
    std::uint64_t output;
    ahandle.read(&output, H5::PredType::NATIVE_UINT64);
    output[0] = sanisizer::cast<std::size_t>(output);

    auto chandle = ghandle.openDataSet("column_names");
    hsize_t clen;
    chandle.getSpace().getSimpleExtentDims(&clen);
    output[1] = sanisizer::cast<std::size_t>(clen);
    return output;
}

}

#endif
