#ifndef TAKANE_GENOMIC_RANGES_HPP
#define TAKANE_GENOMIC_RANGES_HPP

#include "H5Cpp.h"
#include "ritsuko/ritsuko.hpp"
#include "sanisizer/sanisizer.hpp"

#include <string>
#include <filesystem>
#include <stdexcept>
#include <cstdint>
#include <type_traits>
#include <limits>
#include <optional>

#include "utils_string.hpp"
#include "utils_public.hpp"
#include "utils_other.hpp"
#include "utils_json.hpp"
#include "utils_missing.hpp"

/**
 * @file genomic_ranges.hpp
 * @brief Validation for genomic ranges.
 */

namespace takane {

/**
 * @cond
 */
void validate(const std::filesystem::path&, const ObjectMetadata&, const Options& options);
bool derived_from(const std::string&, const std::string&, const Options& options);

struct SequenceLimits {
    SequenceLimits(std::size_t n) : 
        circular(sanisizer::cast<I<decltype(circular.size())> >(n)),
        length(sanisizer::cast<I<decltype(length.size())> >(n))
    {}

    std::vector<std::optional<bool> > circular;
    std::vector<std::optional<std::uint64_t> > length;
};

inline SequenceLimits find_sequence_limits(const std::filesystem::path& path, const Options& options) {
    const std::string type_name = "sequence_information";

    auto smeta = read_object_metadata(path);
    if (!derived_from(smeta.type, type_name, options)) {
        throw std::runtime_error("expected a 'sequence_information' object or one of its subclasses");
    }
    ::takane::validate(path, smeta, options);

    // No need for checks here, we assume everything is now valid.
    H5::H5File handle(path / "info.h5", H5F_ACC_RDONLY);
    auto ghandle = handle.openGroup(type_name);

    auto lhandle = ghandle.openDataSet("length");
    auto lspace = lhandle.getSpace();
    hsize_t num_seq;
    lspace.getSimpleExtentDims(&num_seq);

    SequenceLimits output(num_seq);
    ritsuko::hdf5::Stream1dNumericDatasetOptions opt;
    opt.contiguous_chunk_size = options.hdf5_buffer_size;

    ritsuko::hdf5::Stream1dNumericDataset<std::uint64_t> lstream(&lhandle, num_seq, opt);
    auto lmissing = read_numeric_missing_placeholder<std::uint64_t>(lhandle, "missing-value-placeholder");
    iterate_stream<std::uint64_t>(
        lstream,
        [&](hsize_t i, std::uint64_t l) -> void {
            if (!lmissing.has_value() || l != *lmissing) {
                output.length[i] = l;
            }
        }
    );

    auto chandle = ghandle.openDataSet("circular");
    ritsuko::hdf5::Stream1dNumericDataset<std::int32_t> cstream(&chandle, num_seq, opt);
    auto cmissing = read_numeric_missing_placeholder<std::int32_t>(chandle, "missing-value-placeholder");
    iterate_stream<std::int32_t>(
        cstream,
        [&](hsize_t i, std::int32_t c) -> void {
            if (!cmissing.has_value() || c != *cmissing) {
                output.circular[i] = c;
            }
        }
    );

    return output;
}

inline bool end_position_exceeds_int64(std::int64_t start, std::uint64_t width) {
    constexpr std::uint64_t end_limit = std::numeric_limits<std::int64_t>::max();
    if (start > 0) {
        // That is, does 'start + width - 1 > end_limit'?
        // No underflow as 'start - 1 < end_limit', so 'end_limit - (start - 1) > 0'.
        return sanisizer::is_less_than(end_limit - (start - 1), width);
    }

    const std::uint64_t abs_start = static_cast<std::uint64_t>(-(1 + start)) + 1; // effectively '-start' but avoid overflow of the signed type.
    if (abs_start >= width) {
        // end position is 'start + width - 1 == width - abs_start - 1', which would be negative in this case.
        // So it can't possibly exceed end_limit.
        return false;
    }

    return sanisizer::is_less_than(end_limit, width - abs_start - 1);
}
/**
 * @endcond
 */

/**
 * @param path Path to the directory containing the genomic ranges.
 * @param metadata Metadata for the object, typically read from its `OBJECT` file.
 * @param options Validation options.
 */
inline void validate_genomic_ranges(const std::filesystem::path& path, const ObjectMetadata& metadata, const Options& options) {
    const std::string type_name = "genomic_ranges"; // use a separate variable to avoid dangling reference warnings from GCC.

    try {
        const auto& type_meta = extract_json_object(metadata.other, type_name);
        try {
            const auto& vstring = extract_json_version_string(type_meta);
            auto version = ritsuko::parse_version_string(vstring.c_str(), vstring.size(), /* skip_patch = */ true);
            if (version.major != 1) {
                throw std::runtime_error("unsupported version string '" + vstring + "'");
            }
        } catch (...) {
            std::throw_with_nested(std::runtime_error("failed to validate 'version'"));
        }
    } catch (...) {
        std::throw_with_nested(std::runtime_error("failed to read '" + type_name + "' in the object metadata"));
    } 

    // Figuring out the sequence length constraints.
    std::optional<SequenceLimits> limits;
    try {
        limits = find_sequence_limits(path / "sequence_information", options);
    } catch (...) {
        std::throw_with_nested(std::runtime_error("failed to read the 'sequence_information' object"));
    }

    const auto num_sequences = limits->length.size();

    // Now loading all three components.
    hsize_t num_ranges;
    try {
        H5::H5File handle(path / "ranges.h5", H5F_ACC_RDONLY);
        auto ghandle = handle.openGroup(type_name);

        H5::DataSet id_handle;
        try {
            id_handle = ghandle.openDataSet("sequence");
            if (ritsuko::hdf5::exceeds_integer_limit(id_handle, 64, false)) {
                throw std::runtime_error("expected a datatype that fits into a 64-bit unsigned integer");
            }

            auto id_space = id_handle.getSpace();
            if (id_space.getSimpleExtentNdims() != 1) {
                throw std::runtime_error("expected a 1-dimensional dataset");
            }
            id_space.getSimpleExtentDims(&num_ranges);
        } catch (...) {
            std::throw_with_nested(std::runtime_error("failed to validate 'sequence'"));
        }

        H5::DataSet start_handle;
        try {
            start_handle = ghandle.openDataSet("start");
            if (ritsuko::hdf5::exceeds_integer_limit(start_handle, 64, true)) {
                throw std::runtime_error("expected a datatype that fits into a 64-bit signed integer");
            }

            auto start_space = start_handle.getSpace();
            if (start_space.getSimpleExtentNdims() != 1) {
                throw std::runtime_error("expected a 1-dimensional dataset");
            }
            hsize_t num_start;
            start_space.getSimpleExtentDims(&num_start);
            if (num_start != num_ranges) {
                throw std::runtime_error("dataset extent should be the same as that of 'sequence'");
            }
        } catch (...) {
            std::throw_with_nested(std::runtime_error("failed to validate 'start'"));
        }

        H5::DataSet width_handle;
        try {
            width_handle = ghandle.openDataSet("width");
            if (ritsuko::hdf5::exceeds_integer_limit(width_handle, 64, false)) {
                throw std::runtime_error("expected 'width' to have a datatype that fits into a 64-bit unsigned integer");
            }

            auto width_space = width_handle.getSpace();
            if (width_space.getSimpleExtentNdims() != 1) {
                throw std::runtime_error("expected a 1-dimensional dataset");
            }
            hsize_t num_width;
            width_space.getSimpleExtentDims(&num_width);
            if (num_width != num_ranges) {
                throw std::runtime_error("dataset extent should be the same as that of 'sequence'");
            }
        } catch (...) {
            std::throw_with_nested(std::runtime_error("failed to validate 'width'"));
        }

        ritsuko::hdf5::Stream1dNumericDatasetOptions opt;
        opt.contiguous_chunk_size = options.hdf5_buffer_size;
        NumericStreamIterator<std::uint64_t> id_stream(ritsuko::hdf5::Stream1dNumericDataset<std::uint64_t>(&id_handle, num_ranges, opt));
        NumericStreamIterator<std::int64_t> start_stream(ritsuko::hdf5::Stream1dNumericDataset<std::int64_t>(&start_handle, num_ranges, opt));
        NumericStreamIterator<std::uint64_t> width_stream(ritsuko::hdf5::Stream1dNumericDataset<std::uint64_t>(&width_handle, num_ranges, opt));

        for (I<decltype(num_ranges)> i = 0; i < num_ranges; ++i) {
            const auto id = id_stream.next();
            if (sanisizer::is_greater_than_or_equal(id, num_sequences)) {
                throw std::runtime_error("'sequence' must be less than the number of sequences in 'sequence_information'");
            }

            const auto start = start_stream.next();
            const auto width = width_stream.next();

            // If it's definitely non-circular, the start position should be positive.
            const auto& circular = limits->circular[id];
            if (circular.has_value() && !(*circular)) {
                if (start < 1) {
                    throw std::runtime_error("non-positive 'start' position for a non-circular sequence");
                }

                const auto& length = limits->length[id];
                if (length.has_value()) {
                    if (sanisizer::is_greater_than(start, *length)) {
                        throw std::runtime_error("'start' position exceeds sequence length for a non-circular sequence");
                    }

                    // End position is computed as 'start + width - 1', which should be <= length. 
                    // The LHS should not overflow as 'start >= 1' and 'start <= length' so '0 <= length - start + 1 <= length'.
                    if (sanisizer::is_less_than(*length - (start - 1), width)) {
                        throw std::runtime_error("end position ('start + width - 1') exceeds sequence length for a non-circular sequence");
                    }
                }
            }

            if (end_position_exceeds_int64(start, width)) {
                throw std::runtime_error("end position ('start + width - 1') is beyond the range of a 64-bit signed integer");
            }
        }

        try {       
            auto strand_handle = ghandle.openDataSet("strand");
            if (ritsuko::hdf5::exceeds_integer_limit(strand_handle, 32, true)) {
                throw std::runtime_error("expected a datatype that fits into a 32-bit signed integer");
            }

            auto strand_space = strand_handle.getSpace();
            if (strand_space.getSimpleExtentNdims() != 1) {
                throw std::runtime_error("expected a 1-dimensional dataset");
            }
            hsize_t nstrand;
            strand_space.getSimpleExtentDims(&nstrand);
            if (nstrand != num_ranges) {
                throw std::runtime_error("dataset extent should be the same as that of 'sequence'");
            }

            ritsuko::hdf5::Stream1dNumericDataset<std::int32_t> strand_stream(&strand_handle, num_ranges, opt);
            iterate_stream<std::int32_t>(
                strand_stream,
                [&](hsize_t, std::int32_t x) -> void {
                    if (x < -1 || x > 1) {
                        throw std::runtime_error("entries should be one of 0, -1, or 1");
                    }
                }
            );
        } catch (...) {
            std::throw_with_nested(std::runtime_error("failed to validate 'strand'"));
        }

        validate_names(ghandle, "name", num_ranges, options.hdf5_buffer_size);
    } catch (...) {
        std::throw_with_nested(std::runtime_error("failed to validate '" + type_name + "' in 'ranges.h5'"));
    }

    validate_mcols(path, "range_annotations", num_ranges, options);
    validate_metadata(path, "other_annotations", options);
}

/**
 * @param path Path to a directory containing genomic ranges.
 * @param metadata Metadata for the object, typically read from its `OBJECT` file.
 * @param options Validation options.
 * @return The number of ranges.
 */
inline std::size_t height_of_genomic_ranges(const std::filesystem::path& path, [[maybe_unused]] const ObjectMetadata& metadata, [[maybe_unused]] const Options& options) {
    H5::H5File handle(path / "ranges.h5", H5F_ACC_RDONLY);
    auto ghandle = handle.openGroup("genomic_ranges");
    auto dhandle = ghandle.openDataSet("sequence");
    hsize_t output;
    dhandle.getSpace().getSimpleExtentDims(&output);
    return sanisizer::cast<std::size_t>(output);
}

}

#endif
