#ifndef TAKANE_UTILS_FACTOR_HPP
#define TAKANE_UTILS_FACTOR_HPP

#include <unordered_set>
#include <string>
#include <cstdint>
#include <vector>
#include <stdexcept>
#include <optional>

#include "H5Cpp.h"
#include "ritsuko/ritsuko.hpp"
#include "sanisizer/sanisizer.hpp"

#include "utils_other.hpp"
#include "utils_missing.hpp"

namespace takane {

template<class H5Object_>
void validate_factor_ordered_attribute(const H5Object_& handle) {
    if (!handle.attrExists("ordered")) {
        return;
    }
    auto attr = handle.openAttribute("ordered");
    if (attr.getSpace().getSimpleExtentNdims() != 0) {
        throw std::runtime_error("expected 'ordered' attribute to be a scalar");
    }
    if (ritsuko::hdf5::exceeds_integer_limit(attr, 32, true)) {
        throw std::runtime_error("expected 'ordered' attribute to have a datatype that fits in a 32-bit signed integer");
    }
}

struct DefaultFactorMessenger {
    static std::string level() { return "factor level"; }
    static std::string levels() { return "levels"; }
    static std::string codes() { return "factor codes"; }
};

// These factor level/code checks are useful elsewhere but with different error messages;
// in such cases, we just do some compile-time switches that only affect the error message.
template<class ErrorMessenger_ = DefaultFactorMessenger>
hsize_t validate_factor_levels(const H5::DataSet& handle, hsize_t buffer_size) {
    if (!ritsuko::hdf5::is_utf8_string(handle)) {
        throw std::runtime_error("expected a datatype that can be represented by a UTF-8 encoded string");
    }

    auto lspace = handle.getSpace();
    if (lspace.getSimpleExtentNdims() != 1) {
        throw std::runtime_error("expected a 1-dimensional dataset");
    }
    hsize_t len;
    lspace.getSimpleExtentDims(&len);

    ritsuko::hdf5::Stream1dStringDataset stream(
        &handle,
        len,
        [&]{
            ritsuko::hdf5::Stream1dStringDatasetOptions opt;
            opt.contiguous_chunk_size = buffer_size;
            return opt;
        }()
    );

    std::unordered_set<std::string> present;
    iterate_stream<std::string>(
        stream,
        [&](hsize_t, std::string x) -> void {
            if (present.find(x) != present.end()) {
                throw std::runtime_error("detected duplicated " + ErrorMessenger_::level() + " '" + x + "'");
            }
            present.insert(std::move(x));
        }
    );

    return len;
}

template<class ErrorMessenger_ = DefaultFactorMessenger, typename NumLevels_>
hsize_t validate_factor_codes(const H5::DataSet& handle, NumLevels_ num_levels, hsize_t buffer_size, bool allow_missing) {
    if (ritsuko::hdf5::exceeds_integer_limit(handle, 64, false)) {
        throw std::runtime_error("expected a datatype that fits in a 64-bit unsigned integer");
    }

    auto cspace = handle.getSpace();
    if (cspace.getSimpleExtentNdims() != 1) {
        throw std::runtime_error("expected a 1-dimensional dataset");
    }
    hsize_t len;
    cspace.getSimpleExtentDims(&len);

    ritsuko::hdf5::Stream1dNumericDataset<std::uint64_t> stream(
        &handle,
        len,
        [&]{
            ritsuko::hdf5::Stream1dNumericDatasetOptions opt;
            opt.contiguous_chunk_size = buffer_size;
            return opt;
        }()
    );

    std::optional<std::uint64_t> missing_placeholder;
    if (allow_missing) {
        missing_placeholder = read_numeric_missing_placeholder<std::uint64_t>(handle, "missing-value-placeholder");
    }

    if (missing_placeholder.has_value()) {
        iterate_stream<std::uint64_t>( 
            stream,
            [&](hsize_t, std::uint64_t x) -> void {
                if (x != *missing_placeholder && sanisizer::is_greater_than_or_equal(x, num_levels)) {
                    throw std::runtime_error("expected " + ErrorMessenger_::codes() + " to be less than the number of " + ErrorMessenger_::levels());
                }
            }
        );
    } else {
        iterate_stream<std::uint64_t>( 
            stream,
            [&](hsize_t, std::uint64_t x) -> void {
                if (sanisizer::is_greater_than_or_equal(x, num_levels)) {
                    throw std::runtime_error("expected " + ErrorMessenger_::codes() + " to be less than the number of " + ErrorMessenger_::levels());
                }
            }
        );
    }

    return len;
}

}

#endif
