#ifndef MOCK_COMPRESSED_LIST_H
#define MOCK_COMPRESSED_LIST_H

#include "H5Cpp.h"
#include "utils.h"

#include <vector>
#include <string>
#include <filesystem>

inline H5::Group mock_compressed_list_partitions(const std::filesystem::path& path, std::string name, const std::vector<int>& lengths) {
    H5::H5File handle(path, H5F_ACC_TRUNC);
    auto ghandle = handle.createGroup(name);
    auto dhandle = ghandle.createDataSet("lengths", H5::PredType::NATIVE_UINT32, create_hdf5_dataspace(lengths.size()));
    dhandle.write(lengths.data(), H5::PredType::NATIVE_INT);
    return ghandle;
}

#endif
