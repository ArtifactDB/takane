#ifndef MOCK_SEQUENCE_INFORMATION_H
#define MOCK_SEQUENCE_INFORMATION_H

#include <vector>
#include <string>
#include <numeric>

#include "H5Cpp.h"
#include "utils.h"

struct SequenceInfo {
    SequenceInfo(std::string name, int length, int circular, std::string genome) :
        name(std::move(name)), length(length), circular(circular), genome(std::move(genome))
    {}
    std::string name;
    int length;
    int circular;
    std::string genome;
};

inline H5::Group mock_sequence_information(const std::filesystem::path& dir, const std::vector<SequenceInfo>& info) {
    initialize_directory_simple(dir, "sequence_information", "1.0");
    H5::H5File handle(dir / "info.h5", H5F_ACC_TRUNC);
    auto ghandle = handle.createGroup("sequence_information");

    std::vector<const char*> ptrs;
    ptrs.reserve(info.size());
    std::vector<int> vals;
    vals.reserve(info.size());

    {
        ptrs.clear();
        for (const auto& seq : info) {
            ptrs.push_back(seq.name.c_str());
        }
        auto nhandle = ghandle.createDataSet("name", H5::StrType(0, H5T_VARIABLE), create_hdf5_dataspace(info.size()));
        nhandle.write(ptrs.data(), H5::StrType(0, H5T_VARIABLE));
    }


    {
        vals.clear();
        for (const auto& seq : info) {
            vals.push_back(seq.length);
        }
        add_hdf5_numeric_dataset(ghandle, "length", H5::PredType::NATIVE_UINT32, vals);
    }

    {
        vals.clear();
        for (const auto& seq : info) {
            vals.push_back(seq.circular);
        }
        add_hdf5_numeric_dataset(ghandle, "circular", H5::PredType::NATIVE_INT8, vals);
    }

    {
        ptrs.clear();
        for (const auto& seq : info) {
            ptrs.push_back(seq.genome.c_str());
        }
        auto gnhandle = ghandle.createDataSet("genome", H5::StrType(0, H5T_VARIABLE), create_hdf5_dataspace(info.size()));
        gnhandle.write(ptrs.data(), H5::StrType(0, H5T_VARIABLE));
    }

    return ghandle;
}

#endif
