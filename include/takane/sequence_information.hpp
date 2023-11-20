#ifndef TAKANE_SEQUENCE_INFORMATION_HPP
#define TAKANE_SEQUENCE_INFORMATION_HPP

#include "ritsuko/hdf5/hdf5.hpp"

#include <filesystem>
#include <stdexcept>
#include <unordered_set>
#include <string>

#include "utils_public.hpp"
#include "utils_hdf5.hpp"

/**
 * @file sequence_information.hpp
 * @brief Validation for sequence information.
 */

namespace takane {

/**
 * @namespace takane::sequence_information
 * @brief Definitions for sequence information objects.
 */
namespace sequence_information {

/**
 * @param path Path to the directory containing the data frame.
 * @param options Validation options, typically for reading performance.
 */
inline void validate(const std::filesystem::path& path, const Options& options) try {
    auto fpath  = path / "info.h5";
    H5::H5File handle(fpath, H5F_ACC_RDONLY);

    const char* parent = "sequence_information";
    if (!handle.exists(parent) || handle.childObjType(parent) != H5O_TYPE_GROUP) {
        throw std::runtime_error("expected an 'sequence_information' group");
    }
    auto ghandle = handle.openGroup(parent);

    size_t nseq = 0;
    {
        auto nhandle = ritsuko::hdf5::get_dataset(ghandle, "name");
        if (nhandle.getTypeClass() != H5T_STRING) {
            throw std::runtime_error("expected a string datatype class for 'name'");
        }

        nseq = ritsuko::hdf5::get_1d_length(nhandle.getSpace(), false);
        std::unordered_set<std::string> collected;
        ritsuko::hdf5::load_1d_string_dataset(
            nhandle,
            nseq,
            options.hdf5_buffer_size,
            [&](size_t, const char* s, size_t l) {
                std::string x(s, s + l);
                if (collected.find(x) != collected.end()) {
                    throw std::runtime_error("detected duplicated sequence name '" + x + "'");
                }
                collected.insert(std::move(x));
            }
        );
    }

    const char* missing_attr_name = "missing-value-placeholder";
    {
        auto lhandle = ritsuko::hdf5::get_dataset(ghandle, "length");
        if (ritsuko::hdf5::exceeds_integer_limit(lhandle, 64, false)) {
            throw std::runtime_error("expected a datatype for 'length' that fits in a 64-bit unsigned integer");
        }
        if (ritsuko::hdf5::get_1d_length(lhandle.getSpace(), false) != nseq) {
            throw std::runtime_error("expected lengths of 'length' and 'name' to be equal");
        }
        if (lhandle.attrExists(missing_attr_name)) {
            ritsuko::hdf5::get_missing_placeholder_attribute(lhandle, missing_attr_name);
        }
    }

    {
        auto chandle = ritsuko::hdf5::get_dataset(ghandle, "circular");
        if (ritsuko::hdf5::exceeds_integer_limit(chandle, 32, true)) {
            throw std::runtime_error("expected a datatype for 'length' that fits in a 32-bit signed integer");
        }
        if (ritsuko::hdf5::get_1d_length(chandle.getSpace(), false) != nseq) {
            throw std::runtime_error("expected lengths of 'length' and 'circular' to be equal");
        }
        if (chandle.attrExists(missing_attr_name)) {
            ritsuko::hdf5::get_missing_placeholder_attribute(chandle, missing_attr_name);
        }
    }

    {
        auto gnhandle = ritsuko::hdf5::get_dataset(ghandle, "genome");
        if (gnhandle.getTypeClass() != H5T_STRING) {
            throw std::runtime_error("expected a string datatype class for 'genome'");
        }
        if (ritsuko::hdf5::get_1d_length(gnhandle.getSpace(), false) != nseq) {
            throw std::runtime_error("expected lengths of 'length' and 'genome' to be equal");
        }
        if (gnhandle.attrExists(missing_attr_name)) {
            ritsuko::hdf5::get_missing_placeholder_attribute(gnhandle, missing_attr_name, /* type_class_only = */ true);
        }
    }
} catch (std::exception& e) {
    throw std::runtime_error("failed to validate 'sequence_information' object at '" + path.string() + "'; " + std::string(e.what()));
}

}

}

#endif
