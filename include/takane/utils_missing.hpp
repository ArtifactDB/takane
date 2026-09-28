#ifndef TAKANE_UTILS_MISSING_HPP
#define TAKANE_UTILS_MISSING_HPP

#include <string>
#include <stdexcept>
#include <optional>

#include "H5Cpp.h"
#include "ritsuko/ritsuko.hpp"

namespace takane {

inline void check_string_missing_placeholder(const H5::DataSet& handle, const char* missing_name) {
    if (!handle.attrExists(missing_name)) {
        return;
    }
    auto ahandle = handle.openAttribute(missing_name);
    if (ahandle.getSpace().getSimpleExtentNdims() != 0) {
        throw std::runtime_error("expected '" + std::string(missing_name) + "' attribute to be scalar");
    }
    if (!ritsuko::hdf5::is_utf8_string(ahandle)) {
        throw std::runtime_error("expected '" + std::string(missing_name) + "' attribute to be a UTF-8 string");
    }
    ritsuko::hdf5::validate_scalar_string(ahandle);
}

inline std::optional<std::string> read_string_missing_placeholder(const H5::DataSet& handle, const char* missing_name) {
    if (!handle.attrExists(missing_name)) {
        return std::optional<std::string>();
    }
    auto ahandle = handle.openAttribute(missing_name);
    if (ahandle.getSpace().getSimpleExtentNdims() != 0) {
        throw std::runtime_error("expected '" + std::string(missing_name) + "' attribute to be scalar");
    }
    if (!ritsuko::hdf5::is_utf8_string(ahandle)) {
        throw std::runtime_error("expected '" + std::string(missing_name) + "' attribute to be a UTF-8 string");
    }
    return ritsuko::hdf5::read_scalar_string(ahandle);
}

inline void check_numeric_missing_placeholder(const H5::DataSet& handle, const char* missing_name) {
    if (!handle.attrExists(missing_name)) {
        return;
    }
    auto ahandle = handle.openAttribute(missing_name);
    if (ahandle.getSpace().getSimpleExtentNdims() != 0) {
        throw std::runtime_error("expected '" + std::string(missing_name) + "' attribute to be scalar");
    }
    if (ahandle.getDataType() != handle.getDataType()) {
        throw std::runtime_error("expected '" + std::string(missing_name) + "' attribute to have the same datatype as '" + ritsuko::hdf5::get_name(handle) + "'");
    }
}

template<typename Type_>
std::optional<Type_> read_numeric_missing_placeholder(const H5::DataSet& handle, const char* missing_name) {
    if (!handle.attrExists(missing_name)) {
        return std::optional<Type_>();
    }
    auto ahandle = handle.openAttribute(missing_name);
    if (ahandle.getSpace().getSimpleExtentNdims() != 0) {
        throw std::runtime_error("expected '" + std::string(missing_name) + "' attribute to be scalar");
    }
    if (ahandle.getDataType() != handle.getDataType()) {
        throw std::runtime_error("expected '" + std::string(missing_name) + "' attribute to have the same datatype as '" + ritsuko::hdf5::get_name(handle) + "'");
    }
    Type_ output;
    ahandle.read(ritsuko::hdf5::as_numeric_datatype<Type_>(), &output);
    return output;
}

}

#endif
