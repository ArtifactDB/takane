#ifndef TAKANE_UTILS_STRING_HPP
#define TAKANE_UTILS_STRING_HPP

#include <string>
#include <cstddef>
#include <vector>
#include <stdexcept>
#include <optional>

#include "ritsuko/ritsuko.hpp"
#include "sanisizer/sanisizer.hpp"

#include "utils_other.hpp"

namespace takane {

inline std::string open_and_load_scalar_string_attribute(const H5::Attribute& attr) {
    if (attr.getSpace().getSimpleExtentNdims() != 0) {
        throw std::runtime_error("expected a scalar attribute");
    }
    if (!ritsuko::hdf5::is_utf8_string(attr)) {
        throw std::runtime_error("expected a datatype that can be represented by a UTF-8 encoded string");
    }
    return ritsuko::hdf5::read_scalar_string(attr);
}

inline std::string open_and_load_string_format(const H5::H5Object& handle) {
    if (!handle.attrExists("format")) {
        return "none";
    } 
    try {
        auto fhandle = handle.openAttribute("format");
        return open_and_load_scalar_string_attribute(fhandle);
    } catch (...) {
        std::throw_with_nested(std::runtime_error("failed to validate the 'format' attribute"));
    }
}

template<bool date_>
inline void validate_dates_or_times(const H5::DataSet& handle, hsize_t len, const std::optional<std::string>& missing_value, hsize_t buffer_size) {
    ritsuko::hdf5::Stream1dStringDataset stream(
        &handle,
        len, 
        [&]{
            ritsuko::hdf5::Stream1dStringDatasetOptions opt;
            opt.contiguous_chunk_size = buffer_size;
            return opt;
        }()
    );

    auto validate_string = [&](hsize_t, const std::string& x) -> void {
        if constexpr(date_) {
            if (!ritsuko::is_date(x.c_str(), x.size())) {
                throw std::runtime_error("expected a date-formatted string (got '" + x + "')");
            }
        } else {
            if (!ritsuko::is_rfc3339(x.c_str(), x.size())) {
                throw std::runtime_error("expected a date/time-formatted string (got '" + x + "')");
            }
        }
    };

    if (missing_value.has_value()) {
        iterate_stream<std::string>(
            stream,
            [&](hsize_t i, const std::string& x) -> void {
                if (x != *missing_value) {
                    validate_string(i, x);
                }
            }
        );
    } else {
        iterate_stream<std::string>(stream, validate_string);
    }
}

inline void validate_string_format(
    const H5::DataSet& handle,
    hsize_t len,
    const std::string& format,
    const std::optional<std::string>& missing_value,
    hsize_t buffer_size
) {
    if (format == "date") {
        validate_dates_or_times<true>(handle, len, missing_value, buffer_size);
    } else if (format == "date-time") {
        validate_dates_or_times<false>(handle, len, missing_value, buffer_size);
    } else if (format == "none") {
        ritsuko::hdf5::validate_1d_strings(
            handle,
            len,
            [&]{
                ritsuko::hdf5::Validate1dStringsOptions opt;
                opt.contiguous_chunk_size = buffer_size;
                return opt;
            }()
        );
    } else {
        throw std::runtime_error("unsupported format '" + format + "'");
    }
}

template<typename NumExpected_>
void validate_names(const H5::Group& handle, const std::string& name, NumExpected_ num_expected, hsize_t buffer_size) {
    if (!handle.exists(name)) {
        return;
    }

    try {
        auto nhandle = handle.openDataSet(name);
        if (!ritsuko::hdf5::is_utf8_string(nhandle)) {
            throw std::runtime_error("expected a datatype that can be represented by a UTF-8 encoded string");
        }

        auto nspace = nhandle.getSpace();
        if (nspace.getSimpleExtentNdims() != 1) {
            throw std::runtime_error("expected a 1-dimensional dataset");
        }
        hsize_t nlen;
        nspace.getSimpleExtentDims(&nlen);

        if (!sanisizer::is_equal(num_expected, nlen)) {
            throw std::runtime_error("number of names is not consistent with the height of its parent object");
        }

        ritsuko::hdf5::validate_1d_strings(
            nhandle,
            nlen,
            [&]{
                ritsuko::hdf5::Validate1dStringsOptions opt;
                opt.contiguous_chunk_size = buffer_size;
                return opt;
            }()
        );
    } catch (...) {
        std::throw_with_nested(std::runtime_error("failed to validate '" + name + "'"));
    }
}

}

#endif
