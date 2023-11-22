#ifndef TAKANE_COMPRESSED_LIST_HPP
#define TAKANE_COMPRESSED_LIST_HPP

#include "H5Cpp.h"
#include "ritsuko/ritsuko.hpp"
#include "ritsuko/hdf5/hdf5.hpp"

#include <cstdint>
#include <string>
#include <stdexcept>
#include <vector>
#include <filesystem>

#include "utils_public.hpp"
#include "utils_hdf5.hpp"
#include "utils_other.hpp"

namespace takane {

void validate(const std::filesystem::path&, const std::string&, const Options&);
size_t height(const std::filesystem::path&, const std::string&, const Options&);
bool satisfies_interface(const std::string&, const std::string&);

namespace compressed_list {

template<bool satisfies_interface_>
void validate(const std::filesystem::path& path, const std::string& object_type, const std::string& concatenated_type, const Options& options) try {
    auto handle = ritsuko::hdf5::open_file(path / "partitions.h5");
    auto ghandle = ritsuko::hdf5::open_group(handle, object_type.c_str());

    auto vstring = ritsuko::hdf5::open_and_load_scalar_string_attribute(ghandle, "version");
    auto version = ritsuko::parse_version_string(vstring.c_str(), vstring.size(), /* skip_patch = */ true);
    if (version.major != 1) {
        throw std::runtime_error("unsupported version string '" + vstring + "'");
    }

    auto catdir = path / "concatenated";
    auto cattype = read_object_type(catdir);
    if constexpr(satisfies_interface_) {
        if (!satisfies_interface(cattype, concatenated_type)) {
            throw std::runtime_error("'concatenated' should satisfy the '" + concatenated_type + "' interface");
        }
    } else {
        if (cattype != concatenated_type) {
            throw std::runtime_error("'concatenated' should contain an 'atomic_vector' object");
        }
    }

    try {
        validate(catdir, cattype, options);
    } catch (std::exception& e) {
        throw std::runtime_error("failed to validate the 'concatenated' object; " + std::string(e.what()));
    }
    size_t catheight = height(catdir, cattype, options);

    size_t len = internal_hdf5::validate_compressed_list(ghandle, catheight, options.hdf5_buffer_size);

    internal_hdf5::validate_names(ghandle, "names", len, options.hdf5_buffer_size);
    internal_other::validate_mcols(path, "element_annotations", len, options);
    internal_other::validate_metadata(path, "other_annotations", options);

} catch (std::exception& e) {
    throw std::runtime_error("failed to validate an '" + object_type + "' object at '" + path.string() + "'; " + std::string(e.what()));
}

inline size_t height(const std::filesystem::path& path, const std::string& name, [[maybe_unused]] const Options& options) {
    H5::H5File handle(path / "partitions.h5", H5F_ACC_RDONLY);
    auto ghandle = handle.openGroup(name);
    auto dhandle = ghandle.openDataSet("lengths");
    return ritsuko::hdf5::get_1d_length(dhandle, false);
}

}

}

#endif
