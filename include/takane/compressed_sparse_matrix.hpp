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

    ritsuko::Version version;
    try {
        const auto& type_meta = extract_json_object(metadata.other, type_name);
        try {
            const auto& vstring = extract_json_version_string(type_meta);
            version = ritsuko::parse_version_string(vstring.c_str(), vstring.size(), /* skip_patch = */ true);
            if (version.major != 1) {
                throw std::runtime_error("unsupported version '" + vstring + "'");
            }
        } catch (...) {
            std::throw_with_nested(std::runtime_error("failed to validate 'version'"));
        }
    } catch (...) {
        std::throw_with_nested(std::runtime_error("failed to validate '" + type_name + "' in the object metadata"));
    }

    try {
        H5::H5File handle(path / "matrix.h5", H5F_ACC_RDONLY);
        auto ghandle = handle.openGroup(type_name);

        std::string type;
        try {
            auto thandle = ghandle.openAttribute("type");
            type = open_and_load_scalar_string_attribute(thandle);
        } catch (...) {
            std::throw_with_nested(std::runtime_error("failed to validate the 'type' attribute"));
        }

        bool is_csr = true;
        try {
            auto lhandle = ghandle.openAttribute("layout");
            auto layout = open_and_load_scalar_string_attribute(lhandle);
            if (layout == "CSC") {
                is_csr = false;
            } else if (layout != "CSR") {
                throw std::runtime_error("'layout' should be either 'CSC' or 'CSR'");
            }
        } catch (...) {
            std::throw_with_nested(std::runtime_error("failed to validate the 'layout' attribute"));
        }

        // Checking the shape.
        std::vector<std::uint64_t> shape(2);
        try {
            auto shandle = ghandle.openDataSet("shape");
            if (ritsuko::hdf5::exceeds_integer_limit(shandle, 64, false)) {
                throw std::runtime_error("expected datatype to be a subset of a 64-bit unsigned integer");
            }

            auto sspace = shandle.getSpace();
            if (sspace.getSimpleExtentNdims() != 1) {
                throw std::runtime_error("expected a 1-dimensional dataset");
            }
            hsize_t len;
            sspace.getSimpleExtentDims(&len);
            if (len != 2) {
                throw std::runtime_error("expected a dataset of length 2");
            }

            shandle.read(shape.data(), H5::PredType::NATIVE_UINT64);
        } catch (...) {
            std::throw_with_nested(std::runtime_error("failed to validate 'shape'"));
        }

        // Checking data, and getting the number of structural non-zeros.
        hsize_t num_nonzero;
        try {
            auto dhandle = ghandle.openDataSet("data");

            auto dspace = dhandle.getSpace();
            if (dspace.getSimpleExtentNdims() != 1) {
                throw std::runtime_error("expected a 1-dimensional dataset");
            }
            dspace.getSimpleExtentDims(&num_nonzero);

            if (type == "integer") {
                if (ritsuko::hdf5::exceeds_integer_limit(dhandle, 32, true)) {
                    throw std::runtime_error("expected datatype to fit inside a 32-bit signed integer");
                }
            } else if (type == "boolean") {
                if (ritsuko::hdf5::exceeds_integer_limit(dhandle, 32, true)) {
                    throw std::runtime_error("expected datatype to fit inside a 32-bit signed integer");
                }
            } else if (type == "number") {
                if (ritsuko::hdf5::exceeds_float_limit(dhandle, 64)) {
                    throw std::runtime_error("expected datatype to fit inside a 64-bit float");
                }
            } else {
                throw std::runtime_error("unknown matrix type '" + type + "'");
            }

            validate_numeric_missing_placeholder(dhandle, "missing-value-placeholder");
        } catch (...) {
            std::throw_with_nested(std::runtime_error("failed to validate 'data'"));
        }

        // Checking and extracting the indptrs.
        const auto primary_dim = (is_csr ? shape[0] : shape[1]);
        std::vector<std::uint64_t> indptrs(sanisizer::sum<typename std::vector<std::uint64_t>::size_type>(primary_dim, 1));
        try {
            auto phandle = ghandle.openDataSet("indptr");
            if (ritsuko::hdf5::exceeds_integer_limit(phandle, 64, false)) {
                throw std::runtime_error("expected datatype to be a subset of a 64-bit unsigned integer");
            }

            auto pspace = phandle.getSpace();
            if (pspace.getSimpleExtentNdims() != 1) {
                throw std::runtime_error("expected a 1-dimensional dataset");
            }
            hsize_t numptrs;
            pspace.getSimpleExtentDims(&numptrs);
            if (!sanisizer::is_equal(numptrs, indptrs.size())) {
                auto dimname = (is_csr ? std::string("rows") : std::string("columns"));
                throw std::runtime_error("dataset should have length equal to the number of " + dimname + " plus 1");
            }

            phandle.read(indptrs.data(), H5::PredType::NATIVE_UINT64);
            if (indptrs[0] != 0) {
                throw std::runtime_error("first entry should be zero");
            }
            if (!sanisizer::is_equal(indptrs.back(), num_nonzero)) {
                throw std::runtime_error("last entry should equal the number of non-zero elements");
            }

            for (I<decltype(primary_dim)> i = 1; i < primary_dim; ++i) {
                if (indptrs[i] < indptrs[i-1]) {
                    throw std::runtime_error("pointers should be sorted in increasing order");
                }
            }
        } catch (...) {
            std::throw_with_nested(std::runtime_error("failed to validate 'indptr'"));
        }

        // Finally checking the indices.
        try {
            auto ihandle = ghandle.openDataSet("indices");
            if (ritsuko::hdf5::exceeds_integer_limit(ihandle, 64, false)) {
                throw std::runtime_error("expected datatype to be a subset of a 64-bit unsigned integer");
            }

            auto ispace = ihandle.getSpace();
            if (ispace.getSimpleExtentNdims() != 1) {
                throw std::runtime_error("expected a 1-dimensional dataset");
            }
            hsize_t len;
            ispace.getSimpleExtentDims(&len);
            if (num_nonzero != len) {
                throw std::runtime_error("length of dataset should be equal to the number of non-zero elements");
            }

            NumericStreamIterator<std::uint64_t> indstream(
                ritsuko::hdf5::Stream1dNumericDataset<std::uint64_t>(
                    &ihandle,
                    len,
                    [&]{
                        ritsuko::hdf5::Stream1dNumericDatasetOptions opt;
                        opt.contiguous_chunk_size = options.hdf5_buffer_size;
                        return opt;
                    }()
                )
            );

            const auto secondary_dim = (is_csr ? shape[1] : shape[0]);
            for (I<decltype(primary_dim)> i = 0; i < primary_dim; ++i) {
                const auto start = indptrs[i];
                const auto end = indptrs[i + 1];
                if (start == end) {
                    continue;
                }

                auto previous = indstream.next();
                for (I<decltype(start)> j = start + 1; j < end; ++j) {
                    auto i = indstream.next();
                    if (previous >= i) {
                        auto dimname = (is_csr ? std::string("rows") : std::string("columns"));
                        throw std::runtime_error("entries should be strictly increasing within each " + dimname);
                    }
                    previous = i;
                }

                if (sanisizer::is_greater_than_or_equal(previous, secondary_dim)) {
                    auto dimname = (is_csr ? std::string("columns") : std::string("rows"));
                    throw std::runtime_error("entries should be less than the number of " + dimname);
                }
            }
        } catch (...) {
            std::throw_with_nested(std::runtime_error("failed to validate 'indices'"));
        }

        validate_array_dimnames(ghandle, "names", shape, options);
    } catch (...) {
        std::throw_with_nested(std::runtime_error("failed to validate '" + type_name + "' in 'matrix.h5'"));
    }
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
