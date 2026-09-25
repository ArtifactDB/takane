#ifndef UTILS_H
#define UTILS_H

#include <filesystem>
#include <string>
#include <fstream>
#include <vector>
#include <algorithm>
#include <cstring>

#include "H5Cpp.h"
#include "millijson/millijson.hpp"
#include "ritsuko/ritsuko.hpp"

#include "takane/utils_public.hpp"

//void test_validate(const std::filesystem::path&);
//void test_validate(const std::filesystem::path&, takane::Options& opts);
//
//size_t test_height(const std::filesystem::path&);
//size_t test_height(const std::filesystem::path&, takane::Options& opts);
//
//std::vector<size_t> test_dimensions(const std::filesystem::path&);
//std::vector<size_t> test_dimensions(const std::filesystem::path&, takane::Options& opts);

std::filesystem::path define_test_path(const std::filesystem::path& stub) {
    const std::filesystem::path dir = "TEST-OBJECTS";
    if (!std::filesystem::exists(dir)) {
        std::filesystem::create_directory(dir);
    }
    return dir / stub;
}

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

inline void quick_text_write(const std::string& path, const char* msg) {
    std::ofstream handle(path);
    handle << msg;
}

inline void quick_text_write(const std::string& path, const std::string& msg) {
    std::ofstream handle(path);
    handle << msg;
}

inline void quick_gzip_write(const std::string& path, const char* msg) {
    byteme::GzipFileWriter handle(path.c_str(), {});
    handle.write(reinterpret_cast<const unsigned char*>(msg), std::strlen(msg));
}

inline void quick_gzip_write(const std::string& path, const std::string& msg) {
    byteme::GzipFileWriter handle(path.c_str(), {});
    handle.write(reinterpret_cast<const unsigned char*>(msg.c_str()), msg.size());
}

template<class Handle_>
void add_hdf5_attribute(Handle_& handle, const std::string& name, const std::string& value) {
    H5::StrType stype(0, value.size());
    auto attr = handle.createAttribute(name, stype, H5S_SCALAR);
    attr.write(stype, value);
}

inline H5::DataSet add_hdf5_dataset(H5::Group& handle, const std::string& name, const H5::DataType& dtype, const hsize_t len) {
    H5::DataSpace dspace(1, &len);
    return handle.createDataSet(name, dtype, dspace);
}

inline void dump_json(const millijson::Base* ptr, std::ostream& output) {
    if (ptr->type() == millijson::ARRAY) {
        const auto& vals = reinterpret_cast<const millijson::Array*>(ptr)->value();
        output << "[";
        bool first = true;
        for (const auto& x : vals) {
            if (!first) {
                output << ", ";
            }
            dump_json(x.get(), output);
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
            dump_json(vals.find(n)->second.get(), output);
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

inline void dump_json(const millijson::Base* ptr, const std::filesystem::path& path) {
    std::ofstream output(path);
    dump_json(ptr, output);
}

#endif
