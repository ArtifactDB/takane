#ifndef TAKANE_UTILS_HDF5_HPP
#define TAKANE_UTILS_HDF5_HPP

#include <unordered_set>
#include <string>
#include <cstdint>
#include <vector>
#include <stdexcept>

#include "ritsuko/ritsuko.hpp"
#include "ritsuko/hdf5/hdf5.hpp"

namespace takane {

namespace internal_hdf5 {

template<class H5Object_>
std::string fetch_format_attribute(const H5Object_& handle) {
    if (!handle.attrExists("format")) {
        return "none";
    }

    auto attr = handle.openAttribute("format");
    if (!ritsuko::hdf5::is_scalar(attr)) {
        throw std::runtime_error("expected 'format' attribute to be a scalar");
    }
    if (attr.getTypeClass() != H5T_STRING) {
        throw std::runtime_error("expected 'format' attribute to be a string");
    }
    return ritsuko::hdf5::load_scalar_string_attribute(attr);
}

template<class H5Object_>
void check_ordered_attribute(const H5Object_& handle) {
    if (!handle.attrExists("ordered")) {
        return;
    }

    auto attr = handle.openAttribute("ordered");
    if (!ritsuko::hdf5::is_scalar(attr)) {
        throw std::runtime_error("expected 'ordered' attribute to be a scalar");
    }
    if (ritsuko::hdf5::exceeds_integer_limit(attr, 32, true)) {
        throw std::runtime_error("expected 'ordered' attribute to have a datatype that fits in a 32-bit signed integer");
    }
}

inline void validate_string_format(const H5::DataSet& handle, hsize_t len, const std::string& format, bool has_missing, const std::string& missing_value, hsize_t buffer_size) {
    if (format == "date") {
        ritsuko::hdf5::Stream1dStringDataset stream(&handle, len, buffer_size);
        for (hsize_t i = 0; i < len; ++i, stream.next()) {
            auto x = stream.steal();
            if (has_missing && missing_value == x) {
                continue;
            }
            if (!ritsuko::is_date(x.c_str(), x.size())) {
                throw std::runtime_error("expected a date-formatted string (got '" + x + "')");
            }
        }

    } else if (format == "date-time") {
        ritsuko::hdf5::Stream1dStringDataset stream(&handle, len, buffer_size);
        for (hsize_t i = 0; i < len; ++i, stream.next()) {
            auto x = stream.steal();
            if (has_missing && missing_value == x) {
                continue;
            }
            if (!ritsuko::is_rfc3339(x.c_str(), x.size())) {
                throw std::runtime_error("expected a date/time-formatted string (got '" + x + "')");
            }
        }

    } else if (format == "none") {
        ritsuko::hdf5::validate_1d_string_dataset(handle, len, buffer_size);

    } else {
        throw std::runtime_error("unsupported format '" + format + "'");
    }
}

inline hsize_t validate_factor_levels(const H5::Group& handle, const std::string& name, hsize_t buffer_size) {
    auto lhandle = ritsuko::hdf5::open_dataset(handle, name.c_str());
    if (lhandle.getTypeClass() != H5T_STRING) {
        throw std::runtime_error("expected a string datatype for '" + name + "'");
    }

    auto len = ritsuko::hdf5::get_1d_length(lhandle.getSpace(), false);
    std::unordered_set<std::string> present;

    ritsuko::hdf5::Stream1dStringDataset stream(&lhandle, len, buffer_size);
    for (hsize_t i = 0; i < len; ++i, stream.next()) {
        auto x = stream.steal();
        if (present.find(x) != present.end()) {
            throw std::runtime_error("'" + name + "' contains duplicated factor level '" + x + "'");
        }
        present.insert(std::move(x));
    }

    return len;
}

inline hsize_t validate_factor_codes(const H5::Group& handle, const std::string& name, hsize_t num_levels, hsize_t buffer_size, bool allow_missing = true) {
    auto chandle = ritsuko::hdf5::open_dataset(handle, name.c_str());
    if (ritsuko::hdf5::exceeds_integer_limit(chandle, 32, true)) {
        throw std::runtime_error("expected a datatype for '" + name + "' that fits in a 32-bit signed integer");
    }

    bool has_missing = false;
    int32_t missing_placeholder = 0;
    if (allow_missing) {
        auto missingness = ritsuko::hdf5::open_and_load_optional_numeric_missing_placeholder<int32_t>(chandle, "missing-value-placeholder");
        has_missing = missingness.first;
        missing_placeholder = missingness.second;
    }

    auto len = ritsuko::hdf5::get_1d_length(chandle.getSpace(), false);
    ritsuko::hdf5::Stream1dNumericDataset<int32_t> stream(&chandle, len, buffer_size);
    for (hsize_t i = 0; i < len; ++i, stream.next()) {
        auto x = stream.get();
        if (has_missing && x == missing_placeholder) {
            continue;
        }
        if (x < 0) {
            throw std::runtime_error("expected factor codes to be non-negative");
        }
        if (static_cast<hsize_t>(x) >= num_levels) {
            throw std::runtime_error("expected factor codes to be less than the number of levels");
        }
    }

    return len;
}

inline void validate_names(const H5::Group& handle, const std::string& name, size_t len, hsize_t buffer_size) {
    if (!handle.exists(name)) {
        return;
    }

    auto nhandle = ritsuko::hdf5::open_dataset(handle, name.c_str());
    if (nhandle.getTypeClass() != H5T_STRING) {
        throw std::runtime_error("'" + name + "' should be a string datatype class");
    }

    auto nlen = ritsuko::hdf5::get_1d_length(nhandle.getSpace(), false);
    if (len != nlen) {
        throw std::runtime_error("'" + name + "' should have the same length as the parent object (got " + std::to_string(nlen) + ", expected " + std::to_string(len) + ")");
    }

    ritsuko::hdf5::validate_1d_string_dataset(nhandle, len, buffer_size);
}

}

}

#endif
