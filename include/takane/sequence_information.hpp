#ifndef TAKANE_SEQUENCE_INFORMATION_HPP
#define TAKANE_SEQUENCE_INFORMATION_HPP

#include "H5Cpp.h"
#include "ritsuko/ritsuko.hpp"

#include <filesystem>
#include <stdexcept>
#include <unordered_set>
#include <string>

#include "utils_public.hpp"
#include "utils_json.hpp"
#include "utils_missing.hpp"
#include "utils_other.hpp"

/**
 * @file sequence_information.hpp
 * @brief Validation for sequence information.
 */

namespace takane {

/**
 * @param path Path to the directory containing the data frame.
 * @param metadata Metadata for the object, typically read from its `OBJECT` file.
 * @param options Validation options.
 */
inline void validate_sequence_information(const std::filesystem::path& path, const ObjectMetadata& metadata, const Options& options) {
    const std::string type_name = "sequence_information"; // use a separate variable to avoid dangling reference warnings from GCC.

    try {
        const auto& type_meta = extract_json_object(metadata.other, type_name);
        try {
            const auto& vstring = extract_json_version_string(type_meta);
            auto version = ritsuko::parse_version_string(vstring.c_str(), vstring.size(), /* skip_patch = */ true);
            if (version.major != 1) {
                throw std::runtime_error("unsupported version string '" + vstring + "'");
            }
        } catch (...) {
            std::throw_with_nested(std::runtime_error("failed to read 'version'"));
        }
    } catch (...) {
        std::throw_with_nested(std::runtime_error("failed to validate 'sequence_information' in the object metadata"));
    }

    try {
        H5::H5File handle(path / "info.h5", H5F_ACC_RDONLY);
        auto ghandle = handle.openGroup(type_name);

        hsize_t nseq = 0;
        try {
            auto nhandle = ghandle.openDataSet("name");
            if (!ritsuko::hdf5::is_utf8_string(nhandle)) {
                throw std::runtime_error("expected a datatype that can be represented by a UTF-8 encoded string");
            }

            auto nspace = nhandle.getSpace();
            if (nspace.getSimpleExtentNdims() != 1) {
                throw std::runtime_error("expected a 1-dimensional dataset");
            }
            nspace.getSimpleExtentDims(&nseq);

            ritsuko::hdf5::Stream1dStringDataset stream(
                &nhandle,
                nseq, 
                [&]{
                    ritsuko::hdf5::Stream1dStringDatasetOptions opt;
                    opt.contiguous_chunk_size = options.hdf5_buffer_size;
                    return opt;
                }()
            );

            std::unordered_set<std::string> collected;
            iterate_stream<std::string>(
                stream,
                [&](hsize_t, std::string x) -> void {
                    if (collected.find(x) != collected.end()) {
                        throw std::runtime_error("detected duplicated sequence name '" + x + "'");
                    }
                    collected.insert(std::move(x));
                }
            );
        } catch (...) {
            std::throw_with_nested(std::runtime_error("failed to validate 'name'"));
        }

        try {
            auto lhandle = ghandle.openDataSet("length");
            if (ritsuko::hdf5::exceeds_integer_limit(lhandle, 64, false)) {
                throw std::runtime_error("expected a datatype that fits in a 64-bit unsigned integer");
            }

            auto lspace = lhandle.getSpace();
            if (lspace.getSimpleExtentNdims() != 1) {
                throw std::runtime_error("expected a 1-dimensional dataset");
            }
            hsize_t nlen;
            lspace.getSimpleExtentDims(&nlen);
            if (nlen != nseq) {
                throw std::runtime_error("dataset extent should the same as that of 'name'");
            }

            validate_numeric_missing_placeholder(lhandle, "missing-value-placeholder");
        } catch (...) {
            std::throw_with_nested(std::runtime_error("failed to validate 'length'"));
        }

        try {
            auto chandle = ghandle.openDataSet("circular");
            if (ritsuko::hdf5::exceeds_integer_limit(chandle, 32, true)) {
                throw std::runtime_error("expected a datatype that fits in a 32-bit signed integer");
            }

            auto cspace = chandle.getSpace();
            if (cspace.getSimpleExtentNdims() != 1) {
                throw std::runtime_error("expected a 1-dimensional dataset");
            }
            hsize_t ncirc;
            cspace.getSimpleExtentDims(&ncirc);
            if (ncirc != nseq) {
                throw std::runtime_error("dataset extent should the same as that of 'name'");
            }

            validate_numeric_missing_placeholder(chandle, "missing-value-placeholder");
        } catch (...) {
            std::throw_with_nested(std::runtime_error("failed to validate 'circular'"));
        }

        try {
            auto gnhandle = ghandle.openDataSet("genome");
            if (!ritsuko::hdf5::is_utf8_string(gnhandle)) {
                throw std::runtime_error("expected a datatype that can be represented by a UTF-8 encoded string");
            }

            auto gnspace = gnhandle.getSpace();
            if (gnspace.getSimpleExtentNdims() != 1) {
                throw std::runtime_error("expected a 1-dimensional dataset");
            }
            hsize_t ngen;
            gnspace.getSimpleExtentDims(&ngen);
            if (ngen != nseq) {
                throw std::runtime_error("dataset extent should the same as that of 'name'");
            }

            validate_string_missing_placeholder(gnhandle, "missing-value-placeholder");
        } catch (...) {
            std::throw_with_nested(std::runtime_error("failed to validate 'circular'"));
        }
    } catch (...) {
        std::throw_with_nested(std::runtime_error("failed to validate 'sequence_information' in 'info.h5'"));
    }
}

}

#endif
