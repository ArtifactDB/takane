#ifndef TAKANE_UTILS_BUMPY_ARRAY_HPP
#define TAKANE_UTILS_BUMPY_ARRAY_HPP

#include "H5Cpp.h"
#include "ritsuko/ritsuko.hpp"
#include "sanisizer/sanisizer.hpp"

#include <cstdint>
#include <string>
#include <stdexcept>
#include <vector>
#include <filesystem>

#include "utils_public.hpp"
#include "utils_array.hpp"
#include "utils_other.hpp"
#include "utils_json.hpp"

namespace takane {

void validate(const std::filesystem::path&, const ObjectMetadata&, const Options&);
std::size_t height(const std::filesystem::path&, const ObjectMetadata&, const Options&);
bool satisfies_interface(const std::string&, const std::string&, const Options&);
bool derived_from(const std::string&, const std::string&, const Options&);

inline void validate_bumpy_array_sparse_indices(const H5::Group& handle, const std::vector<std::uint64_t>& dimensions, hsize_t num_lengths, hsize_t buffer_size) {
    const auto ndims = dimensions.size();
    std::vector<H5::DataSet> handles;
    handles.reserve(ndims);

    for (I<decltype(ndims)> d = 0; d < ndims; ++d) {
        auto dname = ritsuko::itoa(d);
        try {
            auto lhandle = handle.openDataSet(dname);
            if (ritsuko::hdf5::exceeds_integer_limit(lhandle, 64, false)) {
                throw std::runtime_error("expected a datatype that fits in a 64-bit unsigned integer");
            }

            auto lspace = lhandle.getSpace();
            if (lspace.getSimpleExtentNdims() != 1) {
                throw std::runtime_error("expected a 1-dimensional dataset");
            }

            hsize_t len;
            lspace.getSimpleExtentDims(&len);
            if (!sanisizer::is_equal(len, num_lengths)) {
                throw std::runtime_error("dataset should have the same length as 'lengths'");
            }
            handles.push_back(lhandle);
        } catch (...) {
            std::throw_with_nested(std::runtime_error("failed to load indices for dimension " + dname));
        }
    }

    // Skip orderedness checks if there are no indices at all.
    if (num_lengths == 0) {
        return;
    }

    std::vector<StreamIterator<std::uint64_t> > streams;
    streams.reserve(ndims);
    std::vector<std::uint64_t> position;
    position.reserve(ndims); 

    for (I<decltype(ndims)> d = 0; d < ndims; ++d) {
        streams.emplace_back(ritsuko::hdf5::Stream1dNumericDataset<std::uint64_t>(
            &(handles[d]),
            num_lengths,
            [&]{
                ritsuko::hdf5::Stream1dNumericDatasetOptions opt;
                opt.contiguous_chunk_size = buffer_size;
                return opt;
            }()
        ));
        position.push_back(streams.back().next());
        if (sanisizer::is_greater_than_or_equal(position.back(), dimensions[d])) {
            throw std::runtime_error("indices for dimension " + ritsuko::itoa(d) + " should be less than the dimension extent");
        }
    }

    for (I<decltype(num_lengths)> l = 1; l < num_lengths; ++l) {
        bool greater = false;

        // Going from back to front and checking for increasingness, based on the
        // expectation that the first dimension is the fastest-changing.
        for (auto rd = ndims; rd > 0; --rd) {
            auto d = rd - 1;
            auto& dstream = streams[d];

            auto nextv = dstream.next();
            if (nextv >= dimensions[d]) {
                throw std::runtime_error("indices for dimension " + ritsuko::itoa(d) + " should be less than the dimension extent");
            }

            auto& oldv = position[d];
            if (!greater) {
                if (nextv > oldv) {
                    greater = true;
                } else if (nextv < oldv) {
                    throw std::runtime_error("coordinates should be strictly increasing");
                }
            }
            oldv = nextv;
        }

        if (!greater) {
            throw std::runtime_error("detected duplicate coordinates");
        }
    }
}

template<bool satisfies_interface_>
void validate_bumpy_array(
    const std::filesystem::path& path,
    const std::string& object_type,
    const std::string& concatenated_type,
    const ObjectMetadata& metadata,
    const Options& options
) {
    try {
        const auto& objmap = extract_json_object(metadata.other, object_type);
        try {
            const auto& vstring = extract_json_version_string(objmap);
            auto version = ritsuko::parse_version_string(vstring.c_str(), vstring.size(), /* skip_patch = */ true);
            if (version.major != 1) {
                throw std::runtime_error("unsupported version string '" + vstring + "'");
            }
        } catch (...) {
            std::throw_with_nested(std::runtime_error("failed to read 'version'"));
        }
    } catch (...) {
        std::throw_with_nested(std::runtime_error("failed to validate '" + object_type + "' in the object metadata"));
    }

    auto catdir = path / "concatenated";
    std::size_t catheight;
    try {
        auto catmeta = read_object_metadata(catdir);
        if constexpr(satisfies_interface_) {
            if (!satisfies_interface(catmeta.type, concatenated_type, options)) {
                throw std::runtime_error("object should satisfy the '" + concatenated_type + "' interface");
            }
        } else {
            if (!derived_from(catmeta.type, concatenated_type, options)) {
                throw std::runtime_error("object should be derived from the '" + concatenated_type + "' type");
            }
        }
        validate(catdir, catmeta, options);
        catheight = height(catdir, catmeta, options);
    } catch (...) {
        std::throw_with_nested(std::runtime_error("failed to validate 'concatenated'"));
    }

    try {
        H5::H5File handle(path / "partitions.h5", H5F_ACC_RDONLY);
        auto ghandle = handle.openGroup(object_type);

        std::vector<std::uint64_t> dims;
        try {
            auto dhandle = ghandle.openDataSet("dimensions");
            if (ritsuko::hdf5::exceeds_integer_limit(dhandle, 64, false)) {
                throw std::runtime_error("expected a datatype that fits in a 64-bit unsigned integer");
            }

            auto dspace = dhandle.getSpace();
            if (dspace.getSimpleExtentNdims() != 1) {
                throw std::runtime_error("expected a 1-dimensional dataset");
            }
            hsize_t len;
            dspace.getSimpleExtentDims(&len);

            sanisizer::resize(dims, len);
            dhandle.read(dims.data(), H5::PredType::NATIVE_UINT64);
        } catch (...) {
            std::throw_with_nested(std::runtime_error("failed to read 'dimensions'"));
        }

        hsize_t num_lengths;
        try {
            auto lhandle = ghandle.openDataSet("lengths");
            if (ritsuko::hdf5::exceeds_integer_limit(lhandle, 64, false)) {
                throw std::runtime_error("expected a datatype that fits in a 64-bit unsigned integer");
            }

            auto lspace = lhandle.getSpace();
            if (lspace.getSimpleExtentNdims() != 1) {
                throw std::runtime_error("expected a 1-dimensional dataset");
            }

            lspace.getSimpleExtentDims(&num_lengths);
            ritsuko::hdf5::Stream1dNumericDataset<std::uint64_t> stream(
                &lhandle,
                num_lengths,
                [&]{
                    ritsuko::hdf5::Stream1dNumericDatasetOptions opt;
                    opt.contiguous_chunk_size = options.hdf5_buffer_size;
                    return opt;
                }()
            );

            std::size_t total = 0;
            iterate_stream<std::uint64_t>(
                stream,
                [&](hsize_t, std::uint64_t val) -> void {
                    total = sanisizer::sum<std::size_t>(total, val);
                }
            );
            if (!sanisizer::is_equal(total, catheight)) {
                throw std::runtime_error("sum of lengths does not equal the height of the concatenated object");
            }
        } catch (...) {
            std::throw_with_nested(std::runtime_error("failed to read 'lengths'"));
        }

        // Support both dense and sparse cases.
        if (ghandle.exists("indices")) {
            try {
                validate_bumpy_array_sparse_indices(ghandle.openGroup("indices"), dims, num_lengths, options.hdf5_buffer_size);
            } catch (...) {
                std::throw_with_nested(std::runtime_error("failed to read sparse 'indices'"));
            }
        } else {
            std::size_t prod = !dims.empty(); // prod = 0 if dims is empty, otherwise it starts at 1.
            for (auto d : dims) {
                prod = sanisizer::product<std::size_t>(prod, d);
            }
            if (!sanisizer::is_equal(prod, num_lengths)) {
                throw std::runtime_error("length of 'lengths' should equal the product of 'dimensions'");
            }
        }

        if (ghandle.exists("names")) {
            validate_array_dimnames(ghandle, "names", dims, options);
        }
    } catch (...) {
        std::throw_with_nested(std::runtime_error("failed to validate '" + object_type + "' in 'partitions.h5'"));
    }
}

inline std::size_t height_of_bumpy_array(
    const std::filesystem::path& path,
    const std::string& name,
    [[maybe_unused]] const ObjectMetadata& metadata,
    [[maybe_unused]] const Options& options
) {
    H5::H5File handle(path / "partitions.h5", H5F_ACC_RDONLY);
    auto ghandle = handle.openGroup(name);
    auto dhandle = ghandle.openDataSet("dimensions");
    auto dspace = dhandle.getSpace();

    hsize_t len;
    dspace.getSimpleExtentDims(&len);
    auto dims = sanisizer::create<std::vector<std::uint64_t> >(len);
    dhandle.read(dims.data(), H5::PredType::NATIVE_UINT64);
    return sanisizer::cast<std::size_t>(dims[0]);
}

inline std::vector<std::size_t> dimensions_of_bumpy_array(
    const std::filesystem::path& path,
    const std::string& name,
    [[maybe_unused]] const ObjectMetadata& metadata,
    [[maybe_unused]] const Options& options
) {
    H5::H5File handle(path / "partitions.h5", H5F_ACC_RDONLY);
    auto ghandle = handle.openGroup(name);
    auto dhandle = ghandle.openDataSet("dimensions");
    auto dspace = dhandle.getSpace();

    hsize_t len;
    dspace.getSimpleExtentDims(&len);
    auto dims = sanisizer::create<std::vector<std::uint64_t> >(len);
    dhandle.read(dims.data(), H5::PredType::NATIVE_UINT64);
    return cast_array_dimensions<std::size_t>(dims);
}

}

#endif
