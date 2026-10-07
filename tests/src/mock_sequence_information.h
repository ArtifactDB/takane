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
        auto nhandle = add_hdf5_dataset(ghandle, "name", H5::StrType(0, H5T_VARIABLE), info.size());
        nhandle.write(ptrs.data(), H5::StrType(0, H5T_VARIABLE));
    }


    {
        vals.clear();
        for (const auto& seq : info) {
            vals.push_back(seq.length);
        }
        auto lhandle = add_hdf5_dataset(ghandle, "length", H5::PredType::NATIVE_UINT32, info.size());
        lhandle.write(vals.data(), H5::PredType::NATIVE_INT);
    }

    {
        vals.clear();
        for (const auto& seq : info) {
            vals.push_back(seq.circular);
        }
        auto chandle = add_hdf5_dataset(ghandle, "circular", H5::PredType::NATIVE_INT8, info.size());
        chandle.write(vals.data(), H5::PredType::NATIVE_INT);
    }

    {
        ptrs.clear();
        for (const auto& seq : info) {
            ptrs.push_back(seq.genome.c_str());
        }
        auto gnhandle = add_hdf5_dataset(ghandle, "genome", H5::StrType(0, H5T_VARIABLE), info.size());
        gnhandle.write(ptrs.data(), H5::StrType(0, H5T_VARIABLE));
    }

    return ghandle;
}

#endif
