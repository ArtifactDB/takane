#ifndef UTILS_H
#define UTILS_H

#include <filesystem>
#include <string>
#include <fstream>
#include <vector>

#include "H5Cpp.h"
#include "takane/takane.hpp"

inline void initialize_directory(const std::filesystem::path& dir) {
    if (std::filesystem::exists(dir)) {
        std::filesystem::remove_all(dir);
    }
    std::filesystem::create_directory(dir);
}

inline void dump_object_metadata(const std::filesystem::path& dir, const std::string& contents) {
    std::ofstream handle(dir / "OBJECT");
    handle << contents;
}

inline std::string generate_metadata_simple(const std::string& name, const std::string& version) {
    return "{ \"type\": \"" + name + "\", \"" + name + "\": { \"version\": \"" + version + "\" } }";
}

inline void dump_object_metadata_simple(const std::filesystem::path& dir, const std::string& name, const std::string& version) {
    std::ofstream handle(dir / "OBJECT");
    handle << generate_metadata_simple(name, version);
}

inline void initialize_directory(const std::filesystem::path& dir, const std::string& contents) {
    initialize_directory(dir);
    dump_object_metadata(dir, contents);
}

inline void initialize_directory_simple(const std::filesystem::path& dir, const std::string& name, const std::string& version) {
    initialize_directory(dir);
    dump_object_metadata_simple(dir, name, version);
}

template<typename ... Args_>
void expect_validation_error(const std::filesystem::path& dir, const std::string& msg, Args_&& ... args) {
    EXPECT_ANY_THROW({
        try {
            takane::validate(dir, std::forward<Args_>(args)...);
        } catch (std::exception& e) {
            EXPECT_THAT(e.what(), ::testing::HasSubstr(msg));
            throw;
        }
    });
}

namespace hdf5_utils {

template<class Handle>
void attach_attribute(Handle& handle, const std::string& name, const std::string& type) {
    H5::StrType stype(0, H5T_VARIABLE);
    auto attr = handle.createAttribute(name, stype, H5S_SCALAR);
    attr.write(stype, type);
}

template<class Handle>
void attach_attribute(Handle& handle, const std::string& name, int val) {
    auto attr = handle.createAttribute(name, H5::PredType::NATIVE_INT32, H5S_SCALAR);
    attr.write(H5::PredType::NATIVE_INT, &val);
}

inline H5::DataSet spawn_data(H5::Group& handle, const std::string& name, hsize_t len, const H5::DataType& dtype) {
    H5::DataSpace dspace(1, &len);
    return handle.createDataSet(name, dtype, dspace);
}

template<typename Type_>
H5::DataSet spawn_numeric_data(H5::Group& handle, const std::string& name, const H5::DataType& dtype, const std::vector<Type_>& values) {
    auto dhandle = spawn_data(handle, name, values.size(), dtype);
    dhandle.write(values.data(), ritsuko::hdf5::as_numeric_datatype<Type_>());
    return dhandle;
}

template<class Container_>
std::vector<const char*> pointerize_strings(const Container_& x) {
    std::vector<const char*> output;
    for (auto start = x.begin(), end = x.end(); start != end; ++start) {
        output.push_back(start->c_str());
    }
    return output;
}

inline H5::DataSet spawn_string_data(H5::Group& handle, const std::string& name, size_t strlen, const std::vector<std::string>& values) {
    auto dhandle = spawn_data(handle, name, values.size(), H5::StrType(0, strlen));

    if (strlen == H5T_VARIABLE) {
        auto ptrs = pointerize_strings(values);
        dhandle.write(ptrs.data(), dhandle.getStrType());
    } else {
        std::vector<char> buffer(strlen * values.size());
        auto bIt = buffer.data();
        for (size_t i = 0; i < values.size(); ++i) {
            std::copy_n(values[i].begin(), std::min(strlen, values[i].size()), bIt);
            bIt += strlen;
        }
        dhandle.write(buffer.data(), dhandle.getStrType());
    }

    return dhandle;
}

}

#endif
