#ifndef TAKANE_GENOMIC_RANGES_HPP
#define TAKANE_GENOMIC_RANGES_HPP

#include "ritsuko/ritsuko.hpp"
#include "comservatory/comservatory.hpp"

#include "WrappedOption.hpp"

#include <string>
#include <filesystem>
#include <stdexcept>
#include <cstdint>

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
 * @param path Path to the directory containing the data frame.
 * @param options Validation options, typically for reading performance.
 */
inline void validate(const std::filesystem::path& path, const Options& options) try {
    std::vector<unsigned char> restricted;
    std::vector<uint64_t> seqlen;

    // Figuring out the sequence length constraints.
    {
        auto spath = path / "sequence_information";
        if (read_object_type(spath) != "sequence_information") {
            throw std::runtime_error("'sequence_information' directory should contain a 'sequence_information' object");
        }
        ::takane::validate(path, options);

        auto fpath  = spath / "info.h5";
        H5::H5File handle(fpath, H5F_ACC_RDONLY);
        auto ghandle = handle.openGroup("sequence_information");

        {
            auto lhandle = ghandle.openDataSet("length");
            auto len = ritsuko::hdf5::get_1d_length(lhandle, false);
            auto block_size = ritsuko::hdf5::pick_1d_block_size(lhandle.getCreatePlist(), len, options.hdf5_buffer_size);
            std::vector<uint64_t> buffer(block_size);
            ritsuko::hdf5::iterate_blocks(
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
            auto len = ritsuko::hdf5::get_1d_length(lhandle, false);
            auto block_size = ritsuko::hdf5::pick_1d_block_size(lhandle.getCreatePlist(), len, options.hdf5_buffer_size);
            std::vector<int32_t> buffer(block_size);

            ritsuko::hdf5::iterate_blocks(
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
    }

    // Now loading all three components.
    auto rpath = path / "ranges.h5";
    H5::H5File rhandle(rpath, H5F_ACC_RDONLY);

    const char* parent = "genomic_ranges";
    if (!handle.exists(parent) || handle.childObjType(parent) != H5O_TYPE_GROUP) {
        throw std::runtime_error("expected an 'sequence_information' group");
    }
    auto ghandle = handle.openGroup(parent);

    std::vector<uint64_t> seq_id, width;
    const char* name_name = "name";
    const char* width_name = "width";

    for (size_t i = 0; i < 2; ++i) {
        const char* name;
        std::vector<uint64_t>* ptr;
        if (i == 0) {
            name = name_name;
            ptr = &seq_id;
        } else if (i == 1) {
            name = width_name;
            ptr = &width;
        }

        auto nhandle = ghandle.openDataSet(name);
        if (ritsuko::hdf5::exceeds_integer_limit(nhandle, 64, false)) {
            throw std::runtime_error("expected '" + std::string(name) + "' to have a datatype that fits into a 64-bit unsigned integer");
        }

        auto len = ritsuko::hdf5::get_1d_length(nhandle, false);
        if (i > 1 && len != seq_id.size()) {
            throw std::runtime_error("'" + std::string(name) + "' and 'name' should have the same length");
        }
        ptr->reserve(len);

        auto block_size = ritsuko::hdf5::pick_1d_block_size(nhandle.getCreatePlist(), len, options.hdf5_buffer_size);
        std::vector<uint64_t> buffer(block_size);
        ritsuko::hdf5::iterate_blocks(
            len,
            block_size,
            [&](hsize_t, hsize_t len, const H5::DataSpace& memspace, const H5::DataSpace& dataspace) {
                nhandle.read(buffer.data(), H5::PredType::NATIVE_UINT64, memspace, dataspace);
                ptr->insert(ptr->end(), buffer.begin(), buffer.begin() + len);
            }
        );
    }

    std::vector<int64_t> start;
    {       
        const char* name = "start";
        auto nhandle = ghandle.openDataSet(name);
        if (ritsuko::hdf5::exceeds_integer_limit(nhandle, 64, true)) {
            throw std::runtime_error("expected '" + std::string(name) + "' to have a datatype that fits into a 64-bit signed integer");
        }

        auto len = ritsuko::hdf5::get_1d_length(nhandle, false);
        if (len != seq_id.size()) {
            throw std::runtime_error("'" + std::string(name) + "' and 'name' should have the same length");
        }
        start.reserve(len);

        auto block_size = ritsuko::hdf5::pick_1d_block_size(nhandle.getCreatePlist(), len, options.hdf5_buffer_size);
        std::vector<int64_t> buffer(block_size);
        ritsuko::hdf5::iterate_blocks(
            len,
            block_size,
            [&](hsize_t, hsize_t len, const H5::DataSpace& memspace, const H5::DataSpace& dataspace) {
                nhandle.read(buffer.data(), H5::PredType::NATIVE_INT64, memspace, dataspace);
                start.insert(start.end(), buffer.begin(), buffer.begin() + len);
            }
        );
    }

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
                throw std::runtime_error("end position beyond sequence length (" + std::to_string(start[i]) + " + " + std::to_string(width[i]) + " > " + std::to_string(limit) + ") for non-circular sequence");
            }
        }

        // 'end_limit - spos' is always non-negative as 'end_limit' is the largest value of an int64_t and 'spos' is also int64_t.
        // In addition, 'end_limit - spos' cannot overflow a uint64_t as it's just the range of an int64_t at its most extreme.
        if (end_limit - spos < width[i]) {
            throw std::runtime_error("end position beyond the range of a 64-bit integer (" + std::to_string(start[i]) + " + " + std::to_string(width[i]) + ")");
        }
    }

    {       
        const char* name = "strand";
        auto nhandle = ghandle.openDataSet(name);
        if (ritsuko::hdf5::exceeds_integer_limit(nhandle, 32, true)) {
            throw std::runtime_error("expected '" + std::string(name) + "' to have a datatype that fits into a 64-bit signed integer");
        }

        auto len = ritsuko::hdf5::get_1d_length(nhandle, false);
        if (len != seq_id.size()) {
            throw std::runtime_error("'" + std::string(name) + "' and 'name' should have the same length");
        }

        auto block_size = ritsuko::hdf5::pick_1d_block_size(nhandle.getCreatePlist(), len, options.hdf5_buffer_size);
        std::vector<int64_t> buffer(block_size);
        ritsuko::hdf5::iterate_blocks(
            len,
            block_size,
            [&](hsize_t, hsize_t len, const H5::DataSpace& memspace, const H5::DataSpace& dataspace) {
                nhandle.read(buffer.data(), H5::PredType::NATIVE_INT64, memspace, dataspace);
                for (hsize_t i = 0; i < len; ++i) {
                    if (buffer[i] != -1 && buffer[i] != 0 && buffer[i] != 1) {
                        throw std::runtime_error("values of 'strand' should be one of 0, -1, or 1 (got " + std::to_string(buffer[i]) + ")");
                    }
                }
            }
        );
    }


}

/**
 * Checks if a CSV data frame is correctly formatted for genomic ranges.
 * An error is raised if the file does not meet the specifications.
 *
 * @tparam Reader A **byteme** reader class.
 *
 * @param reader A stream of bytes from the CSV file.
 * @param params Validation parameters.
 */
template<class Reader>
void validate(Reader& reader, const Parameters& params) {
    validate_base(
        [&](comservatory::Contents& contents, const comservatory::ReadOptions& opt) -> void { comservatory::read(reader, contents, opt); },
        params
    );
}

/**
 * Checks if a CSV data frame is correctly formatted for genomic ranges.
 * An error is raised if the file does not meet the specifications.
 *
 * @param path Path to the CSV file.
 * @param params Validation parameters.
 */
inline void validate(const char* path, const Parameters& params) {
    validate_base(
        [&](comservatory::Contents& contents, const comservatory::ReadOptions& opt) -> void { comservatory::read_file(path, contents, opt); },
        params
    );
}

}

}

#endif
