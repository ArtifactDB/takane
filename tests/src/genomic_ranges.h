#ifndef GENOMIC_RANGES_H 
#define GENOMIC_RANGES_H

#include <vector>
#include <string>
#include <numeric>

#include "H5Cpp.h"
#include "utils.h"
#include "sequence_information.h"

namespace genomic_ranges {

inline void mock(
    const H5::Group& handle,
    const std::vector<int>& seq_id, 
    const std::vector<int>& start, 
    const std::vector<int>& width, 
    const std::vector<int>& strand)
{
    auto qhandle = hdf5_utils::spawn_data(handle, "sequence", seq_id.size(), H5::PredType::NATIVE_UINT32);
    qhandle.write(seq_id.data(), H5::PredType::NATIVE_INT);

    auto shandle = hdf5_utils::spawn_data(handle, "start", start.size(), H5::PredType::NATIVE_INT32);
    shandle.write(start.data(), H5::PredType::NATIVE_INT);

    auto whandle = hdf5_utils::spawn_data(handle, "width", width.size(), H5::PredType::NATIVE_UINT64);
    whandle.write(width.data(), H5::PredType::NATIVE_INT);

    auto thandle = hdf5_utils::spawn_data(handle, "strand", strand.size(), H5::PredType::NATIVE_INT8);
    thandle.write(strand.data(), H5::PredType::NATIVE_INT);
}

inline void mock(
    const std::filesystem::path& dir, 
    const std::vector<int>& seq_id, 
    const std::vector<int>& start, 
    const std::vector<int>& width, 
    const std::vector<int>& strand,
    const std::vector<int>& seq_length, 
    const std::vector<int>& is_circular)
{
    initialize_directory(dir, "genomic_ranges");
    H5::H5File handle(dir / "ranges.h5", H5F_ACC_TRUNC);
    auto ghandle = handle.createGroup("genomic_ranges");
    mock(ghandle, seq_id, start, width, strand);

    std::vector<std::string> mock_names, mock_genomes;
    for (size_t i = 0; i < seq_length.size(); ++i) {
        mock_names.push_back(std::to_string(i));
        mock_genomes.push_back("mm10");
    }
    sequence_information::mock(dir / "sequence_information", mock_names, seq_length, is_circular, mock_genomes);
}

}

#endif
