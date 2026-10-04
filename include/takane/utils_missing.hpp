#ifndef TAKANE_UTILS_MISSING_HPP
#define TAKANE_UTILS_MISSING_HPP

#include <string>
#include <stdexcept>
#include <optional>

#include "H5Cpp.h"
#include "ritsuko/ritsuko.hpp"

namespace takane {

inline void validate_string_missing_placeholder(const H5::DataSet& handle, const char* missing_name) {
    if (!handle.attrExists(missing_name)) {
        return;
    }

    try {
        auto ahandle = handle.openAttribute(missing_name);
        if (ahandle.getSpace().getSimpleExtentNdims() != 0) {
            throw std::runtime_error("expected a scalar attribute");
        }
        if (!ritsuko::hdf5::is_utf8_string(ahandle)) {
            throw std::runtime_error("expected a UTF-8 string");
        }
        ritsuko::hdf5::validate_scalar_string(ahandle);
    } catch (...) {
        std::throw_with_nested(std::runtime_error("failed to validate the '" + std::string(missing_name) + "' attribute"));
    }
}

inline std::optional<std::string> read_string_missing_placeholder(const H5::DataSet& handle, const char* missing_name) {
    if (!handle.attrExists(missing_name)) {
        return std::optional<std::string>();
    }

    try {
        auto ahandle = handle.openAttribute(missing_name);
        if (ahandle.getSpace().getSimpleExtentNdims() != 0) {
            throw std::runtime_error("expected a scalar attribute");
        }
        if (!ritsuko::hdf5::is_utf8_string(ahandle)) {
            throw std::runtime_error("expected a UTF-8 string");
        }
        return ritsuko::hdf5::read_scalar_string(ahandle);
    } catch (...) {
        std::throw_with_nested(std::runtime_error("failed to read the '" + std::string(missing_name) + "' attribute"));
    }
}

inline void validate_numeric_missing_placeholder(const H5::DataSet& handle, const char* missing_name) {
    if (!handle.attrExists(missing_name)) {
        return;
    }

    try {
        auto ahandle = handle.openAttribute(missing_name);
        if (ahandle.getSpace().getSimpleExtentNdims() != 0) {
            throw std::runtime_error("expected a scalar attribute");
        }
        if (ahandle.getDataType() != handle.getDataType()) {
            throw std::runtime_error("expected attribute to have the same datatype as its parent dataset");
        }
    } catch (...) {
        std::throw_with_nested(std::runtime_error("failed to validate the '" + std::string(missing_name) + "' attribute"));
    }
}

template<typename Type_>
std::optional<Type_> read_numeric_missing_placeholder(const H5::DataSet& handle, const char* missing_name) {
    if (!handle.attrExists(missing_name)) {
        return std::optional<Type_>();
    }

    try {
        auto ahandle = handle.openAttribute(missing_name);
        if (ahandle.getSpace().getSimpleExtentNdims() != 0) {
            throw std::runtime_error("expected a scalar attribute");
        }
        if (ahandle.getDataType() != handle.getDataType()) {
            throw std::runtime_error("expected attribute to have the same datatype as its parent dataset");
        }
        Type_ output;
        ahandle.read(ritsuko::hdf5::as_numeric_datatype<Type_>(), &output);
        return output;
    } catch (...) {
        std::throw_with_nested(std::runtime_error("failed to read the '" + std::string(missing_name) + "' attribute"));
    }
}

}

#endif
