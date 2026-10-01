#ifndef TAKANE_COMPRESSED_SPARSE_MATRIX_HPP
#define TAKANE_COMPRESSED_SPARSE_MATRIX_HPP

#include "H5Cpp.h"
#include "ritsuko/ritsuko.hpp"

#include "utils_public.hpp"
#include "utils_array.hpp"
#include "utils_json.hpp"

#include <filesystem>
#include <stdexcept>
#include <string>
#include <cstdint>
#include <cstddef>
#include <vector>

/**
 * @file compressed_sparse_matrix.hpp
 * @brief Validation for compressed sparse matrices.
 */

namespace takane {

/**
 * @param path Path to a directory containing a compressed sparse matrix.
 * @param metadata Metadata for the object, typically read from its `OBJECT` file.
 * @param options Validation options.
 */
inline void validate_compressed_sparse_matrix(const std::filesystem::path& path, const ObjectMetadata& metadata, const Options& options) {
    const std::string type_name = "compressed_sparse_matrix"; // use a separate variable to avoid dangling reference warnings from GCC.

    const auto& type_meta = extract_json_type_metadata(metadata.other, type_name);
    const auto& vstring = extract_json_version_string(type_meta, type_name);
    auto version = ritsuko::parse_version_string(vstring.c_str(), vstring.size(), /* skip_patch = */ true);
    if (version.major != 1) {
        throw std::runtime_error("unsupported version '" + vstring + "'");
    }

    H5::H5File handle(path / "matrix.h5", H5F_ACC_RDONLY);
    auto ghandle = handle.openGroup(type_name);

    auto type = open_and_load_scalar_string_attribute(ghandle, "type");
    auto layout = open_and_load_scalar_string_attribute(ghandle, "layout");
    bool is_csr = true;
    if (layout == "CSC") {
        is_csr = false;
    } else if (layout != "CSR") {
        throw std::runtime_error("'layout' attribute must be one of 'CSC' or 'CSR'");
    }

    // Checking the shape.
    std::vector<std::uint64_t> shape(2);
    {
        auto shandle = ghandle.openDataSet("shape");
        if (ritsuko::hdf5::exceeds_integer_limit(shandle, 64, false)) {
            throw std::runtime_error("expected the datatype of 'shape' to be a subset of a 64-bit unsigned integer");
        }

        auto sspace = shandle.getSpace();
        if (sspace.getSimpleExtentNdims() != 1) {
            throw std::runtime_error("expected the 'shape' dataset to be 1-dimensional");
        }
        hsize_t len;
        sspace.getSimpleExtentDims(&len);
        if (len != 2) {
            throw std::runtime_error("expected the 'shape' dataset to be of length 2");
        }

        shandle.read(shape.data(), H5::PredType::NATIVE_UINT64);
    }

    // Checking data, and getting the number of structural non-zeros.
    hsize_t num_nonzero;
    {
        auto dhandle = ghandle.openDataSet("data");

        auto dspace = dhandle.getSpace();
        if (dspace.getSimpleExtentNdims() != 1) {
            throw std::runtime_error("expected the 'data' dataset to be 1-dimensional");
        }
        dspace.getSimpleExtentDims(&num_nonzero);

        if (type == "integer") {
            if (ritsuko::hdf5::exceeds_integer_limit(dhandle, 32, true)) {
                throw std::runtime_error("expected an integer 'data' to fit inside a 32-bit signed integer");
            }
        } else if (type == "boolean") {
            if (ritsuko::hdf5::exceeds_integer_limit(dhandle, 32, true)) {
                throw std::runtime_error("expected a boolean 'data' to fit inside a 32-bit signed integer");
            }
        } else if (type == "number") {
            if (ritsuko::hdf5::exceeds_float_limit(dhandle, 64)) {
                throw std::runtime_error("expected a number 'data' to fit inside a 64-bit float");
            }
        } else {
            throw std::runtime_error("unknown matrix type '" + type + "'");
        }

        validate_numeric_missing_placeholder(dhandle, "missing-value-placeholder");
    }

    // Checking and extracting the indptrs.
    const auto primary_dim = (is_csr ? shape[0] : shape[1]);
    std::vector<std::uint64_t> indptrs(sanisizer::sum<typename std::vector<std::uint64_t>::size_type>(primary_dim, 1));
    {
        auto phandle = ghandle.openDataSet("indptr");
        if (ritsuko::hdf5::exceeds_integer_limit(phandle, 64, false)) {
            throw std::runtime_error("expected datatype of 'indptr' to be a subset of a 64-bit unsigned integer");
        }

        auto pspace = phandle.getSpace();
        if (pspace.getSimpleExtentNdims() != 1) {
            throw std::runtime_error("expected the 'indptr' dataset to be 1-dimensional");
        }
        hsize_t numptrs;
        pspace.getSimpleExtentDims(&numptrs);
        if (!sanisizer::is_equal(numptrs, indptrs.size())) {
            auto dimname = (is_csr ? std::string("rows") : std::string("columns"));
            throw std::runtime_error("'indptr' dataset should have length equal to the number of " + dimname + " plus 1");
        }

        phandle.read(indptrs.data(), H5::PredType::NATIVE_UINT64);
        if (indptrs[0] != 0) {
            throw std::runtime_error("first entry of 'indptr' should be zero");
        }
        if (!sanisizer::is_equal(indptrs.back(), num_nonzero)) {
            throw std::runtime_error("last entry of 'indptr' should equal the number of non-zero elements");
        }

        for (I<decltype(primary_dim)> i = 1; i < primary_dim; ++i) {
            if (indptrs[i] < indptrs[i-1]) {
                throw std::runtime_error("pointers in 'indptr' should be sorted in increasing order");
            }
        }
    }

    // Finally checking the indices.
    {
        auto ihandle = ghandle.openDataSet("indices");
        if (ritsuko::hdf5::exceeds_integer_limit(ihandle, 64, false)) {
            throw std::runtime_error("expected datatype of 'indices' to be a subset of a 64-bit unsigned integer");
        }

        auto ispace = ihandle.getSpace();
        if (ispace.getSimpleExtentNdims() != 1) {
            throw std::runtime_error("expected the 'indices' dataset to be 1-dimensional");
        }
        hsize_t len;
        ispace.getSimpleExtentDims(&len);
        if (num_nonzero != len) {
            throw std::runtime_error("length of 'indices' should be equal to the number of non-zero elements");
        }

        ritsuko::hdf5::Stream1dNumericDataset<std::uint64_t> stream(
            &ihandle,
            len,
            [&]{
                ritsuko::hdf5::Stream1dNumericDatasetOptions opt;
                opt.contiguous_chunk_size = options.hdf5_buffer_size;
                return opt;
            }()
        );

        auto buffer = sanisizer::create<std::vector<std::uint64_t> >(stream.chunk_size());
        hsize_t available = 0, at = 0;
        auto next = [&]() -> std::uint64_t {
            if (at == available) {
                at = 0;
                available = stream.load(buffer.data());
            }
            return buffer[at++];
        };

        const auto secondary_dim = (is_csr ? shape[1] : shape[0]);
        for (I<decltype(primary_dim)> i = 0; i < primary_dim; ++i) {
            const auto start = indptrs[i];
            const auto end = indptrs[i + 1];
            if (start == end) {
                continue;
            }

            auto previous = next();
            for (I<decltype(start)> j = start + 1; j < end; ++j) {
                auto i = next();
                if (previous >= i) {
                    auto dimname = (is_csr ? std::string("rows") : std::string("columns"));
                    throw std::runtime_error("indices should be strictly increasing within each " + dimname);
                }
                previous = i;
            }

            if (sanisizer::is_greater_than_or_equal(previous, secondary_dim)) {
                auto dimname = (is_csr ? std::string("columns") : std::string("rows"));
                throw std::runtime_error("entries of 'indices' should be less than the number of " + dimname);
            }
        }
    }

    validate_array_dimnames(ghandle, "names", shape, options);
}

/**
 * @param path Path to the directory containing a compressed sparse matrix.
 * @param metadata Metadata for the object, typically read from its `OBJECT` file.
 * @param options Validation options.
 * @return Number of rows in the matrix.
 */
inline std::size_t height_of_compressed_sparse_matrix(const std::filesystem::path& path, [[maybe_unused]] const ObjectMetadata& metadata, [[maybe_unused]] const Options& options) {
    H5::H5File handle(path / "matrix.h5", H5F_ACC_RDONLY);
    auto ghandle = handle.openGroup("compressed_sparse_matrix");
    auto shandle = ghandle.openDataSet("shape");
    std::vector<std::uint64_t> output(2);
    shandle.read(output.data(), H5::PredType::NATIVE_UINT64);
    return sanisizer::cast<std::size_t>(output.front());
}

/**
 * @param path Path to the directory containing a compressed sparse matrix.
 * @param metadata Metadata for the object, typically read from its `OBJECT` file.
 * @param options Validation options.
 * @return Dimensions of the matrix.
 */
inline std::vector<size_t> dimensions_of_compressed_sparse_matrix(const std::filesystem::path& path, [[maybe_unused]] const ObjectMetadata& metadata, [[maybe_unused]] const Options& options) {
    H5::H5File handle(path / "matrix.h5", H5F_ACC_RDONLY);
    auto ghandle = handle.openGroup("compressed_sparse_matrix");
    auto shandle = ghandle.openDataSet("shape");
    std::vector<std::uint64_t> output(2);
    shandle.read(output.data(), H5::PredType::NATIVE_UINT64);
    return cast_array_dimensions<std::size_t>(output);
}

}

#endif
