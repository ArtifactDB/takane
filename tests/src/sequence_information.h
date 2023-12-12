#ifndef SEQUENCE_INFORMATION_H
#define SEQUENCE_INFORMATION_H

#include <vector>
#include <string>
#include <numeric>

#include "H5Cpp.h"
#include "utils.h"

namespace sequence_information {

inline void mock(
    H5::Group& handle, 
    const std::vector<std::string>& name, 
    const std::vector<int>& length, 
    const std::vector<int>& circular, 
    const std::vector<std::string>& genome)
{
    hdf5_utils::spawn_string_data(handle, "name", H5T_VARIABLE, name);
    auto lhandle = hdf5_utils::spawn_data(handle, "length", length.size(), H5::PredType::NATIVE_UINT32);
    lhandle.write(length.data(), H5::PredType::NATIVE_INT);
    auto chandle = hdf5_utils::spawn_data(handle, "circular", circular.size(), H5::PredType::NATIVE_INT8);
    chandle.write(circular.data(), H5::PredType::NATIVE_INT);
    hdf5_utils::spawn_string_data(handle, "genome", H5T_VARIABLE, genome);
}

inline void mock(
    const std::filesystem::path& dir, 
    const std::vector<std::string>& name, 
    const std::vector<int>& length, 
    const std::vector<int>& circular, 
    const std::vector<std::string>& genome)
{
    initialize_directory_simple(dir, "sequence_information", "1.0");
    H5::H5File handle(dir / "info.h5", H5F_ACC_TRUNC);
    auto ghandle = handle.createGroup("sequence_information");
    mock(ghandle, name, length, circular, genome);
}

}

#endif
