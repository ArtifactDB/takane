#ifndef TAKANE_UTILS_COMPRESSED_LIST_HPP
#define TAKANE_UTILS_COMPRESSED_LIST_HPP

#include "H5Cpp.h"
#include "ritsuko/ritsuko.hpp"
#include "sanisizer/sanisizer.hpp"

#include <cstdint>
#include <cstddef>
#include <string>
#include <stdexcept>
#include <vector>
#include <filesystem>

#include "utils_public.hpp"
#include "utils_string.hpp"
#include "utils_other.hpp"
#include "utils_json.hpp"

namespace takane {

void validate(const std::filesystem::path&, const ObjectMetadata&, const Options&);
std::size_t height(const std::filesystem::path&, const ObjectMetadata&, const Options&);
bool satisfies_interface(const std::string&, const std::string&, const Options&);
bool derived_from(const std::string&, const std::string&, const Options&);

template<bool satisfies_interface_>
void validate_compressed_list(const std::filesystem::path& path, const std::string& object_type, const std::string& concatenated_type, const ObjectMetadata& metadata, const Options& options) {
    const auto& type_meta = extract_json_type_metadata(metadata.other, object_type);
    const auto& vstring = extract_json_version_string(type_meta, object_type);
    auto version = ritsuko::parse_version_string(vstring.c_str(), vstring.size(), /* skip_patch = */ true);
    if (version.major != 1) {
        throw std::runtime_error("unsupported version string '" + vstring + "'");
    }

    auto catdir = path / "concatenated";
    auto catmeta = read_object_metadata(catdir);
    if constexpr(satisfies_interface_) {
        if (!satisfies_interface(catmeta.type, concatenated_type, options)) {
            throw std::runtime_error("'concatenated' should satisfy the '" + concatenated_type + "' interface");
        }
    } else {
        if (!derived_from(catmeta.type, concatenated_type, options)) {
            throw std::runtime_error("'concatenated' should contain an object of type '" + concatenated_type + "'");
        }
    }

    try {
        ::takane::validate(catdir, catmeta, options);
    } catch (std::exception& e) {
        throw std::runtime_error("failed to validate the 'concatenated' object; " + std::string(e.what()));
    }
    const auto catheight = ::takane::height(catdir, catmeta, options);

    H5::H5File handle(path / "partitions.h5", H5F_ACC_RDONLY);
    auto ghandle = handle.openGroup(object_type);
    auto lhandle = ghandle.openDataSet("lengths");
    if (ritsuko::hdf5::exceeds_integer_limit(lhandle, 64, false)) {
        throw std::runtime_error("expected 'lengths' to have a datatype that fits in a 64-bit unsigned integer");
    }

    auto lspace = lhandle.getSpace();
    if (lspace.getSimpleExtentNdims() != 1) {
        throw std::runtime_error("expected 'lengths' to be a 1-dimensional dataset");
    }
    hsize_t len;
    lspace.getSimpleExtentDims(&len);

    ritsuko::hdf5::Stream1dNumericDataset<std::uint64_t> stream(
        &lhandle,
        len,
        [&]{
            ritsuko::hdf5::Stream1dNumericDatasetOptions opt;
            opt.contiguous_chunk_size = options.hdf5_buffer_size;
            return opt;
        }()
    );
    hsize_t total = 0;
    iterate_stream<std::uint64_t>(
        stream, 
        [&](hsize_t, std::uint64_t val) -> void {
            total = sanisizer::sum<hsize_t>(total, val); 
        }
    );

    if (!sanisizer::is_equal(total, catheight)) {
        throw std::runtime_error("sum of 'lengths' does not equal the height of the concatenated object (got " + std::to_string(total) + ", expected " + std::to_string(catheight) + ")");
    }

    validate_names(ghandle, "names", len, options.hdf5_buffer_size);
    validate_mcols(path, "element_annotations", len, options);
    validate_metadata(path, "other_annotations", options);
}

inline std::size_t height_of_compressed_list(const std::filesystem::path& path, const std::string& name, [[maybe_unused]] const ObjectMetadata& metadata, [[maybe_unused]] const Options& options) {
    H5::H5File handle(path / "partitions.h5", H5F_ACC_RDONLY);
    auto ghandle = handle.openGroup(name);
    auto dhandle = ghandle.openDataSet("lengths");
    hsize_t len;
    dhandle.getSpace().getSimpleExtentDims(&len);
    return sanisizer::cast<std::size_t>(len);
}

}

#endif
