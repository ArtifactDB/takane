#ifndef TAKANE_GENOMIC_RANGES_HPP
#define TAKANE_GENOMIC_RANGES_HPP

#include "ritsuko/ritsuko.hpp"
#include "comservatory/comservatory.hpp"

#include "WrappedOption.hpp"

#include <string>
#include <filesystem>
#include <stdexcept>
#include <cstdint>
#include <type_traits>
#include <limits>

/**
 * @file genomic_ranges.hpp
 * @brief Validation for genomic ranges.
 */

namespace takane {

/**
 * @cond
 */
void validate(const std::filesystem::path&, const std::string&, const Options& options);
/**
 * @endcond
 */

/**
 * @namespace takane::genomic_ranges
 * @brief Definitions for genomic ranges.
 */
namespace genomic_ranges {

/**
 * @cond
 */
namespace internal {

struct SequenceLimits {
    std::vector<unsigned char> restricted;
    std::vector<uint64_t> seqlen;
};

inline SequenceLimits find_sequence_limits(const std::filesystem::path& path, const Options& options) {
    auto xtype = read_object_type(path);
    if (xtype != "sequence_information") {
        throw std::runtime_error("'sequence_information' directory should contain a 'sequence_information' object");
    }
    ::takane::validate(path, xtype, options);

    SequenceLimits output;
    auto& restricted = output.restricted;
    auto& seqlen = output.seqlen;

    auto fpath = path / "info.h5";
    H5::H5File handle(fpath, H5F_ACC_RDONLY);
    auto ghandle = handle.openGroup("sequence_information");

    {
        auto lhandle = ghandle.openDataSet("length");
        auto len = ritsuko::hdf5::get_1d_length(lhandle.getSpace(), false);
        seqlen.reserve(len);

        auto block_size = ritsuko::hdf5::pick_1d_block_size(lhandle.getCreatePlist(), len, options.hdf5_buffer_size);
        std::vector<uint64_t> buffer(block_size);
        ritsuko::hdf5::iterate_1d_blocks(
            len,
            block_size,
            [&](hsize_t, hsize_t len, const H5::DataSpace& memspace, const H5::DataSpace& dataspace) {
                lhandle.read(buffer.data(), H5::PredType::NATIVE_UINT64, memspace, dataspace);
                seqlen.insert(seqlen.end(), buffer.begin(), buffer.begin() + len);
            }
        );

        restricted.resize(seqlen.size(), true);
        if (lhandle.exists("missing-value-placeholder")) {
            auto ahandle = ritsuko::hdf5::get_missing_placeholder_attribute(lhandle, "missing-value-placeholder");
            uint64_t placeholder = 0;
            ahandle.read(H5::PredType::NATIVE_UINT64, &placeholder);
            auto rIt = restricted.begin();
            for (auto x : seqlen) {
                *rIt = (x != placeholder);
                ++rIt;
            }
        }
    }

    {
        auto lhandle = ghandle.openDataSet("circular");

        bool has_missing = lhandle.exists("missing-value-placeholder");
        int32_t placeholder = 0;
        if (has_missing) {
            auto ahandle = ritsuko::hdf5::get_missing_placeholder_attribute(lhandle, "missing-value-placeholder");
            ahandle.read(H5::PredType::NATIVE_INT32, &placeholder);
        }

        // This is already validated, so we can assume that the lengths are the same.
        auto len = ritsuko::hdf5::get_1d_length(lhandle.getSpace(), false);
        auto block_size = ritsuko::hdf5::pick_1d_block_size(lhandle.getCreatePlist(), len, options.hdf5_buffer_size);
        std::vector<int32_t> buffer(block_size);

        ritsuko::hdf5::iterate_1d_blocks(
            len,
            block_size,
            [&](hsize_t start, hsize_t len, const H5::DataSpace& memspace, const H5::DataSpace& dataspace) {
                lhandle.read(buffer.data(), H5::PredType::NATIVE_INT32, memspace, dataspace);
                for (hsize_t i = 0; i < len; ++i) {
                    if (has_missing && buffer[i] == placeholder) {
                        ;
                    } else if (buffer[i]) {
                        restricted[i + start] = false;
                    }
                }
            }
        );
    }

    return output;
}

template<typename MaxType, class Function1, class Function2>
std::vector<MaxType> read_vector(const H5::Group& ghandle, const std::string& name, hsize_t buffer_size, Function1 validate_length, Function2 validate_value) {
    constexpr int nbits = std::numeric_limits<MaxType>::digits;
    constexpr bool issign = std::is_signed<MaxType>::value;

    auto nhandle = ghandle.openDataSet(name);
    if (ritsuko::hdf5::exceeds_integer_limit(nhandle, nbits, issign)) {
        throw std::runtime_error("expected '" + std::string(name) + "' to have a datatype that fits into a " + 
            std::to_string(nbits) + "-bit " + (issign ? std::string("signed") : std::string("unsigned")) + " integer");
    }

    auto len = ritsuko::hdf5::get_1d_length(nhandle.getSpace(), false);
    validate_length(len);
    std::vector<MaxType> output;
    output.reserve(len);

    auto block_size = ritsuko::hdf5::pick_1d_block_size(nhandle.getCreatePlist(), len, buffer_size);
    std::vector<MaxType> buffer(block_size);
    ritsuko::hdf5::iterate_1d_blocks(
        len,
        block_size,
        [&](hsize_t, hsize_t len, const H5::DataSpace& memspace, const H5::DataSpace& dataspace) {
            nhandle.read(buffer.data(), H5::PredType::NATIVE_UINT64, memspace, dataspace);
            output.insert(output.end(), buffer.begin(), buffer.begin() + len);
            for (hsize_t i = 0; i < len; ++i) {
                validate_value(buffer[i]);
            }
        }
    );

    return output;
}

}
/**
 * @endcond
 */

/**
 * @param path Path to the directory containing the data frame.
 * @param options Validation options, typically for reading performance.
 */
inline void validate(const std::filesystem::path& path, const Options& options) try {
    // Figuring out the sequence length constraints.
    auto limits = internal::find_sequence_limits(path / "sequence_information", options);
    const auto& restricted = limits.restricted;
    const auto& seqlen = limits.seqlen;
    size_t num_sequences = restricted.size();

    // Now loading all three components.
    auto rpath = path / "ranges.h5";
    H5::H5File handle(rpath, H5F_ACC_RDONLY);

    const char* parent = "genomic_ranges";
    if (!handle.exists(parent) || handle.childObjType(parent) != H5O_TYPE_GROUP) {
        throw std::runtime_error("expected an 'sequence_information' group");
    }
    auto ghandle = handle.openGroup(parent);

    auto seq_id = internal::read_vector<uint64_t>(
        ghandle, 
        "sequence", 
        options.hdf5_buffer_size, 
        [](size_t) {},
        [&](uint64_t x) {
            if (x >= num_sequences) {
                throw std::runtime_error("'sequence' must be less than the number of sequences");
            }
        }
    );

    auto start = internal::read_vector<int64_t>(
        ghandle, 
        "start", 
        options.hdf5_buffer_size, 
        [&](size_t len) {
            if (len != seq_id.size()) {
                throw std::runtime_error("'start' and 'sequence' should have the same length");
            }
        },
        [](int64_t) {}
    );

    auto width = internal::read_vector<uint64_t>(
        ghandle, 
        "name", 
        options.hdf5_buffer_size, 
        [&](size_t len) {
            if (len != seq_id.size()) {
                throw std::runtime_error("'width' and 'sequence' should have the same length");
            }
        },
        [](uint64_t) {}
    );

    constexpr uint64_t end_limit = std::numeric_limits<int64_t>::max();
    for (size_t i = 0, end = seq_id.size(); i < end; ++i) {
        auto id = seq_id[i];

        if (restricted[id]) {
            if (start[i] < 1) {
                throw std::runtime_error("non-positive start position (" + std::to_string(start[i]) + ") for non-circular sequence");
            }

            auto spos = static_cast<uint64_t>(start[i]);
            auto limit = seqlen[id];
            if (spos > limit) {
                throw std::runtime_error("start position beyond sequence length (" + std::to_string(start[i]) + " > " + std::to_string(limit) + ") for non-circular sequence");
            }

            // The LHS should not overflow as 'spos >= 1' so 'limit - spos + 1' should still be no greater than 'limit'.
            if (limit - spos + 1 < width[i]) {
                throw std::runtime_error("end position beyond sequence length (" + 
                    std::to_string(start[i]) + " + " + std::to_string(width[i]) + " > " + std::to_string(limit) + 
                    ") for non-circular sequence");
            }
        }

        // 'end_limit - start[i]' is always non-negative as 'end_limit' is the largest value of an int64_t and 'start[i]' is also int64_t.
        // In addition, 'end_limit - start[i]' cannot overflow a uint64_t as it's just the range of an int64_t at its most extreme.
        bool exceeded = false;
        if (start[i] > 0) {
            exceeded = (end_limit - static_cast<uint64_t>(start[i]) < width[i]);
        } else {
            exceeded = (end_limit + static_cast<uint64_t>(-start[i]) < width[i]);
        }
        if (exceeded) {
            throw std::runtime_error("end position beyond the range of a 64-bit integer (" + std::to_string(start[i]) + " + " + std::to_string(width[i]) + ")");
        }
    }

    {       
        const char* name = "strand";
        auto nhandle = ghandle.openDataSet(name);
        if (ritsuko::hdf5::exceeds_integer_limit(nhandle, 32, true)) {
            throw std::runtime_error("expected '" + std::string(name) + "' to have a datatype that fits into a 64-bit signed integer");
        }

        auto len = ritsuko::hdf5::get_1d_length(nhandle.getSpace(), false);
        if (len != seq_id.size()) {
            throw std::runtime_error("'" + std::string(name) + "' and 'name' should have the same length");
        }

        auto block_size = ritsuko::hdf5::pick_1d_block_size(nhandle.getCreatePlist(), len, options.hdf5_buffer_size);
        std::vector<int64_t> buffer(block_size);
        ritsuko::hdf5::iterate_1d_blocks(
            len,
            block_size,
            [&](hsize_t, hsize_t len, const H5::DataSpace& memspace, const H5::DataSpace& dataspace) {
                nhandle.read(buffer.data(), H5::PredType::NATIVE_INT64, memspace, dataspace);
                for (hsize_t i = 0; i < len; ++i) {
                    if (buffer[i] < -1 || buffer[i] > 1) {
                        throw std::runtime_error("values of 'strand' should be one of 0, -1, or 1 (got " + std::to_string(buffer[i]) + ")");
                    }
                }
            }
        );
    }

    // Checking the names.
    if (ghandle.exists("names")) {
        auto nhandle = ritsuko::hdf5::get_dataset(ghandle, "names");
        if (nhandle.getTypeClass() != H5T_STRING) {
            throw std::runtime_error("'names' should be a string datatype class");
        }
        auto nlen = ritsuko::hdf5::get_1d_length(nhandle.getSpace(), false);
        if (nlen != seq_id.size()) {
            throw std::runtime_error("'names' and 'codes' should have the same length");
        }
    }

    // Checking the metadata.
    try {
        internal_other::validate_mcols(path / "column_annotations", seq_id.size(), options);
    } catch (std::exception& e) {
        throw std::runtime_error("failed to validate 'column_annotations'; " + std::string(e.what()));
    }

    try {
        internal_other::validate_metadata(path / "other_annotations", options);
    } catch (std::exception& e) {
        throw std::runtime_error("failed to validate 'other_annotations'; " + std::string(e.what()));
    }
} catch (std::exception& e) {
    throw std::runtime_error("failed to validate 'genomic_ranges' object at '" + path.string() + "'; " + std::string(e.what()));
}

}

}

#endif
