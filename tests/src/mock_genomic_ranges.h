#ifndef MOCK_GENOMIC_RANGES_H 
#define MOCK_GENOMIC_RANGES_H

#include <vector>
#include <string>
#include <numeric>

#include "H5Cpp.h"
#include "utils.h"
#include "mock_sequence_information.h"

struct GenomicRange {
    GenomicRange(int sequence, int start, int width, int strand) :
        sequence(sequence), start(start), width(width), strand(strand)
    {}
    int sequence;
    int start;
    int width;
    int strand;
};

inline H5::Group mock_genomic_ranges(const std::filesystem::path& dir, const std::vector<GenomicRange>& ranges, const std::vector<SequenceInfo>& sequences) {
    initialize_directory_simple(dir, "genomic_ranges", "1.0");

    H5::H5File handle(dir / "ranges.h5", H5F_ACC_TRUNC);
    auto ghandle = handle.createGroup("genomic_ranges");

    std::vector<int> vals;
    vals.reserve(ranges.size());

    {
        vals.clear();
        for (const auto& rg : ranges) {
            vals.push_back(rg.sequence);
        }
        auto qhandle = add_hdf5_dataset(ghandle, "sequence", H5::PredType::NATIVE_UINT32, ranges.size());
        qhandle.write(vals.data(), H5::PredType::NATIVE_INT);
    }

    {
        vals.clear();
        for (const auto& rg : ranges) {
            vals.push_back(rg.start);
        }
        auto shandle = add_hdf5_dataset(ghandle, "start", H5::PredType::NATIVE_INT32, ranges.size());
        shandle.write(vals.data(), H5::PredType::NATIVE_INT);
    }

    {
        vals.clear();
        for (const auto& rg : ranges) {
            vals.push_back(rg.width);
        }
        auto whandle = add_hdf5_dataset(ghandle, "width", H5::PredType::NATIVE_UINT64, ranges.size());
        whandle.write(vals.data(), H5::PredType::NATIVE_INT);
    }

    {
        vals.clear();
        for (const auto& rg : ranges) {
            vals.push_back(rg.strand);
        }
        auto thandle = add_hdf5_dataset(ghandle, "strand", H5::PredType::NATIVE_INT8, ranges.size());
        thandle.write(vals.data(), H5::PredType::NATIVE_INT);
    }

    mock_sequence_information(dir / "sequence_information", sequences);
    return ghandle;
}

//inline void mock(const std::filesystem::path& dir, hsize_t num_ranges, hsize_t num_seq) {
//    std::vector<int> seq_id, start, width, strand;
//    for (hsize_t i = 0; i < num_ranges; ++i) {
//        seq_id.push_back(i % num_seq);
//        start.push_back(i * 10); 
//        width.push_back((i % 2) * 10 + 1); 
//        strand.push_back((i % 3) - 1);
//    }
//
//    std::vector<int> seq_length, is_circular;
//    for (hsize_t s = 0; s < num_seq; ++s) {
//        seq_length.push_back(num_ranges * 100);
//        is_circular.push_back(s % 5 == 0);
//    }
//
//    mock(dir, seq_id, start, width, strand, seq_length, is_circular);
//}

#endif
