#ifndef MOCK_SEQUENCE_INFORMATION_H
#define MOCK_SEQUENCE_INFORMATION_H

#include <vector>
#include <string>
#include <numeric>

#include "H5Cpp.h"
#include "utils.h"

inline H5::Group mock_sequence_information(
    const std::filesystem::path& dir, 
    const std::vector<std::string>& name, 
    const std::vector<int>& length, 
    const std::vector<int>& circular, 
    const std::vector<std::string>& genome
) {
    initialize_directory_simple(dir, "sequence_information", "1.0");
    H5::H5File handle(dir / "info.h5", H5F_ACC_TRUNC);
    auto ghandle = handle.createGroup("sequence_information");

    {
        auto ptrs = pointerize_strings(name);
        auto nhandle = add_hdf5_dataset(ghandle, "name", H5::StrType(0, H5T_VARIABLE), name.size());
        nhandle.write(ptrs.data(), H5::StrType(0, H5T_VARIABLE));
    }

    {
        auto lhandle = add_hdf5_dataset(ghandle, "length", H5::PredType::NATIVE_UINT32, length.size());
        lhandle.write(length.data(), H5::PredType::NATIVE_INT);
    }

    {
        auto chandle = add_hdf5_dataset(ghandle, "circular", H5::PredType::NATIVE_INT8, circular.size());
        chandle.write(circular.data(), H5::PredType::NATIVE_INT);
    }

    {
        auto ptrs = pointerize_strings(genome);
        auto gnhandle = add_hdf5_dataset(ghandle, "genome", H5::StrType(0, H5T_VARIABLE), genome.size());
        gnhandle.write(ptrs.data(), H5::StrType(0, H5T_VARIABLE));
    }

    return ghandle;
}

#endif
