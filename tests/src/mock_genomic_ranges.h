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

struct GenomicRangesOptions {
    GenomicRangesOptions(std::vector<GenomicRange> ranges, std::vector<SequenceInfo> sequences) :
        ranges(std::move(ranges)),
        sequences(std::move(sequences)),
        version("1.0")
    {}
    GenomicRangesOptions() : GenomicRangesOptions({}, {}) {}
public:
    std::vector<GenomicRange> ranges;
    std::vector<SequenceInfo> sequences;
    std::string version;
};

inline H5::Group mock_genomic_ranges(const std::filesystem::path& dir, const GenomicRangesOptions& options) {
    initialize_directory_simple(dir, "genomic_ranges", options.version);

    H5::H5File handle(dir / "ranges.h5", H5F_ACC_TRUNC);
    auto ghandle = handle.createGroup("genomic_ranges");

    const auto& ranges = options.ranges;
    const auto& sequences = options.sequences;
    std::vector<int> vals;
    vals.reserve(ranges.size());

    {
        vals.clear();
        for (const auto& rg : ranges) {
            vals.push_back(rg.sequence);
        }
        add_hdf5_numeric_dataset(ghandle, "sequence", H5::PredType::NATIVE_UINT32, vals);
    }

    {
        vals.clear();
        for (const auto& rg : ranges) {
            vals.push_back(rg.start);
        }
        add_hdf5_numeric_dataset(ghandle, "start", H5::PredType::NATIVE_INT32, vals);
    }

    {
        vals.clear();
        for (const auto& rg : ranges) {
            vals.push_back(rg.width);
        }
        add_hdf5_numeric_dataset(ghandle, "width", H5::PredType::NATIVE_UINT64, vals);
    }

    {
        vals.clear();
        for (const auto& rg : ranges) {
            vals.push_back(rg.strand);
        }
        add_hdf5_numeric_dataset(ghandle, "strand", H5::PredType::NATIVE_INT8, vals);
    }

    mock_sequence_information(dir / "sequence_information", sequences);
    return ghandle;
}

inline H5::Group mock_genomic_ranges(const std::filesystem::path& dir, hsize_t num_ranges, hsize_t num_seq) {
    GenomicRangesOptions options;

    options.ranges.reserve(num_ranges);
    for (hsize_t i = 0; i < num_ranges; ++i) {
        options.ranges.emplace_back(i % num_seq, i * 10, (i % 2) * 10 + 1, (i % 3) - 1);
    }

    options.sequences.reserve(num_seq);
    for (hsize_t s = 0; s < num_seq; ++s) {
        options.sequences.emplace_back("chr" + std::to_string(s), num_ranges * 100, s % 5 == 0, "foobar");
    }

    return mock_genomic_ranges(dir, options);
}

#endif
