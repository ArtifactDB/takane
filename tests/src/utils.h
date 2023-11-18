#ifndef UTILS_H
#define UTILS_H

#include "takane/utils_csv.hpp"

struct Hdf5Utils {
    template<class Handle>
    static void attach_attribute(const Handle& handle, const std::string& name, const std::string& type) {
        H5::StrType stype(0, H5T_VARIABLE);
        auto attr = handle.createAttribute(name, stype, H5S_SCALAR);
        attr.write(stype, type);
    }

    template<class Handle>
    static void attach_attribute(const Handle& handle, const std::string& name, int val) {
        auto attr = handle.createAttribute(name, H5::PredType::NATIVE_INT32, H5S_SCALAR);
        attr.write(H5::PredType::NATIVE_INT, &val);
    }

    static H5::DataSet spawn_data(const H5::Group& handle, const std::string& name, hsize_t len, const H5::DataType& dtype) {
        H5::DataSpace dspace(1, &len);
        return handle.createDataSet(name, dtype, dspace);
    }

    template<class Container_>
    static std::vector<const char*> pointerize_strings(const Container_& x) {
        std::vector<const char*> output;
        for (auto start = x.begin(), end = x.end(); start != end; ++start) {
            output.push_back(start->c_str());
        }
        return output;
    }

    static H5::DataSet spawn_string_data(const H5::Group& handle, const std::string& name, size_t strlen, const std::vector<std::string>& values) {
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
    

};

#endif
