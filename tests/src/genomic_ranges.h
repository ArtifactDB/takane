#ifndef GENOMIC_RANGES_H 
#define GENOMIC_RANGES_H

#include <vector>
#include <string>
#include <numeric>

#include "H5Cpp.h"
#include "utils.h"

namespace genomic_ranges {

inline void mock(
    const H5::Group& handle, 
    const std::vector<int>& seq_id, 
    const std::vector<int>& start, 
    const std::vector<int>& end, 
    const std::vector<int>& strand)
{
    auto qhandle = hdf5_utils::spawn_data(handle, "sequence", seq_id.size(), H5::PredType::NATIVE_UINT32);
    qhandle.write(seq_id.data(), H5::PredType::NATIVE_INT);

    auto shandle = hdf5_utils::spawn_data(handle, "start", start.size(), H5::PredType::NATIVE_INT32);
    shandle.write(start.data(), H5::PredType::NATIVE_INT);

    auto whandle = hdf5_utils::spawn_data(handle, "width", width.size(), H5::PredType::NATIVE_UINT64);
    whandle.write(width.data(), H5::PredType::NATIVE_INT);

    auto thandle = hdf5_utils::spawn_data(handle, "strand", circular.size(), H5::PredType::NATIVE_INT8);
    thandle.write(strand.data(), H5::PredType::NATIVE_INT);
}

}

#endif

