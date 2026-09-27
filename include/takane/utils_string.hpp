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

template<class H5Object_>
std::string open_and_load_scalar_string_attribute(const H5Object_& handle, const std::string& name) {
    auto attr = handle.openAttribute(name);
    if (attr.getSpace().getSimpleExtentNdims() != 0) {
        throw std::runtime_error("expected '" + name + "' attribute to be a scalar");
    }
    if (!ritsuko::hdf5::is_utf8_string(attr)) {
        throw std::runtime_error("expected '" + name + "' to have a datatype that can be represented by a UTF-8 encoded string");
    }
    return ritsuko::hdf5::read_scalar_string(attr);
}

template<class H5Object_>
std::string open_and_load_string_format(const H5Object_& handle) {
    if (!handle.attrExists("format")) {
        return "none";
    } else {
        return open_and_load_scalar_string_attribute(handle, "format");        
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
    auto buffer = sanisizer::create<std::vector<std::string> >(stream.chunk_size());

    auto check_string = [&](const std::string& x) -> void {
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

    while (true) {
        auto available = stream.load(buffer.data());
        if (available == 0) {
            break;
        }
        if (missing_value.has_value()) {
            for (I<decltype(available)> i = 0; i < available; ++i) {
                const auto& x = buffer[i];
                if (x != *missing_value) {
                    check_string(x);
                }
            }
        } else {
            for (I<decltype(available)> i = 0; i < available; ++i) {
                check_string(buffer[i]);
            }
        }
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

inline void validate_names(const H5::Group& handle, const std::string& name, std::size_t len, hsize_t buffer_size) {
    if (!handle.exists(name)) {
        return;
    }

    auto nhandle = handle.openDataSet(name);
    if (!ritsuko::hdf5::is_utf8_string(nhandle)) {
        throw std::runtime_error("expected '" + name + "' to have a datatype that can be represented by a UTF-8 encoded string");
    }

    auto nspace = nhandle.getSpace();
    if (nspace.getSimpleExtentNdims() != 1) {
        throw std::runtime_error("expected '" + name + "' to be a 1-dimensional dataset");
    }
    hsize_t nlen;
    nspace.getSimpleExtentDims(&nlen);

    if (!sanisizer::is_equal(len, nlen)) {
        throw std::runtime_error("'" + name + "' should have the same length as the parent object (got " + std::to_string(nlen) + ", expected " + std::to_string(len) + ")");
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
}

}

#endif
