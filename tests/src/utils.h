#ifndef UTILS_H
#define UTILS_H

#include <filesystem>
#include <string>
#include <fstream>
#include <vector>
#include <algorithm>

#include "millijson/millijson.hpp"
#include "ritsuko/hdf5/hdf5.hpp"
#include "H5Cpp.h"

#include "takane/utils_public.hpp"

void test_validate(const std::filesystem::path&);
void test_validate(const std::filesystem::path&, takane::Options& opts);

size_t test_height(const std::filesystem::path&);
size_t test_height(const std::filesystem::path&, takane::Options& opts);

std::vector<size_t> test_dimensions(const std::filesystem::path&);
std::vector<size_t> test_dimensions(const std::filesystem::path&, takane::Options& opts);

inline void initialize_directory(const std::filesystem::path& dir) {
    if (std::filesystem::exists(dir)) {
        std::filesystem::remove_all(dir);
    }
    std::filesystem::create_directory(dir);
}

inline void dump_object_metadata_simple(const std::filesystem::path& dir, const std::string& name, const std::string& version) {
    std::ofstream handle(dir / "OBJECT");
    handle << "{ \"type\": \"" << name << "\", \"" << name << "\": { \"version\": \"" << version << "\" } }";
}

inline void initialize_directory_simple(const std::filesystem::path& dir, const std::string& name, const std::string& version) {
    initialize_directory(dir);
    dump_object_metadata_simple(dir, name, version);
}

template<typename ... Args_>
void expect_validation_error(const std::filesystem::path& dir, const std::string& msg, Args_&& ... args) {
    EXPECT_ANY_THROW({
        try {
            test_validate(dir, std::forward<Args_>(args)...);
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

namespace json_utils {

inline void dump(const millijson::Base* ptr, std::ostream& output) {
    if (ptr->type() == millijson::ARRAY) {
        const auto& vals = reinterpret_cast<const millijson::Array*>(ptr)->value();
        output << "[";
        bool first = true;
        for (const auto& x : vals) {
            if (!first) {
                output << ", ";
            }
            dump(x.get(), output);
            first = false;
        }
        output << "]";

    } else if (ptr->type() == millijson::OBJECT) {
        const auto& vals = reinterpret_cast<const millijson::Object*>(ptr)->value();

        // Sorting them so we have a stable output.
        std::vector<std::string> all_names;
        for (const auto& x : vals) {
            all_names.push_back(x.first);
        }
        std::sort(all_names.begin(), all_names.end());

        output << "{";
        bool first = true;
        for (const auto& n : all_names) {
            if (!first) {
                output << ", ";
            }
            output << "\"" << n << "\": "; // hope there's no weird characters in here.
            dump(vals.find(n)->second.get(), output);
            first = false;
        }
        output << "}";

    } else if (ptr->type() == millijson::STRING) {
        const auto& val = reinterpret_cast<const millijson::String*>(ptr)->value();
        output << "\"" << val << "\"";

    } else if (ptr->type() == millijson::NUMBER) {
        const auto& val = reinterpret_cast<const millijson::Number*>(ptr)->value();
        output << val;

    } else if (ptr->type() == millijson::BOOLEAN) {
        const auto& val = reinterpret_cast<const millijson::Boolean*>(ptr)->value();
        if (val) {
            output << "true";
        } else {
            output << "false";
        }

    } else if (ptr->type() == millijson::NOTHING) {
        output << "null";

    } else {
        throw std::runtime_error("unknown millijson type");
    }
}

inline void dump(const millijson::Base* ptr, const std::filesystem::path& path) {
    std::ofstream output(path);
    json_utils::dump(ptr, output);
}

}

#endif
