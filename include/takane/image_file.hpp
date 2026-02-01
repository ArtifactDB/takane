#ifndef TAKANE_IMAGE_FILE_HPP
#define TAKANE_IMAGE_FILE_HPP

#include "utils_files.hpp"

#include <filesystem>
#include <stdexcept>
#include <array>
#include <string>

/**
 * @file image_file.hpp
 * @brief Validation for standard image files.
 */

namespace takane {

/**
 * @namespace takane::image_file
 * @brief Definitions for standard image files.
 */
namespace image_file {

/**
 * @cond
 */
namespace internal {

// Factored out for re-use in spatial_experiment::internal::validate_image.
inline void validate_png(const std::filesystem::path& path) {
    // Magic number from http://www.libpng.org/pub/png/spec/1.2/png-1.2-pdg.html#PNG-file-signature
    constexpr std::array<unsigned char, 8> expected { 137, 80, 78, 71, 13, 10, 26, 10 };
    internal_files::check_signature<byteme::RawFileReader>(path, expected.data(), expected.size(), "PNG");
}

inline void validate_tiff(const std::filesystem::path& path) {
    std::array<unsigned char, 4> observed{};
    internal_files::extract_signature(path, observed.data(), observed.size());
    // Magic numbers from https://en.wikipedia.org/wiki/Magic_number_(programming)
    constexpr std::array<unsigned char, 4> iisig { 0x49, 0x49, 0x2A, 0x00 };
    constexpr std::array<unsigned char, 4> mmsig { 0x4D, 0x4D, 0x00, 0x2A };
    if (observed != iisig && observed != mmsig) {
        throw std::runtime_error("incorrect TIFF file signature for '" + path.string() + "'");
    }
}

}
/**
 * @endcond
 */

/**
 * If `Options::image_file_strict_check` is provided, it is used to perform stricter checking of the image file contents. 
 * By default, we don't look past the magic number to verify the files as this requires a dependency on heavy-duty libraries like, e.g., Magick.
 *
 * @param path Path to the directory containing the image file.
 * @param metadata Metadata for the object, typically read from its `OBJECT` file.
 * @param options Validation options.
 */
inline void validate(const std::filesystem::path& path, const ObjectMetadata& metadata, Options& options) {
    const std::string type_name = "image_file"; // use a separate variable to avoid dangling reference warnings from GCC.
    const auto& obj = internal_json::extract_typed_object_from_metadata(metadata.other, type_name);

    const auto& vstring = internal_json::extract_string_from_typed_object(obj, "version", type_name);
    auto version = ritsuko::parse_version_string(vstring.c_str(), vstring.size(), /* skip_patch = */ true);
    if (version.major != 1) {
        throw std::runtime_error("unsupported version string '" + vstring + "'");
    }

    const std::string format_name = "format";
    const std::string& format = internal_json::extract_string(obj, format_name);
    const std::string prefix = (path / "file").string();

    if (format == "PNG") {
        const std::filesystem::path ipath = prefix + ".png";
        internal::validate_png(ipath);

    } else if (format == "TIFF") {
        const std::filesystem::path ipath = prefix + ".tif";
        internal::validate_tiff(ipath);

    } else if (format == "JPEG") {
        auto ipath = prefix + ".jpg";
        // Common prefix of the JPEG-related magic numbers from https://en.wikipedia.org/wiki/List_of_file_signatures
        constexpr std::array<unsigned char, 2> expected { 0xFF, 0xD8 };
        internal_files::check_signature<byteme::RawFileReader>(ipath, expected.data(), expected.size(), "JPEG");

    } else if (format == "GIF") {
        auto ipath = prefix + ".gif";
        // Common prefix of the old and new magic numbers from https://en.wikipedia.org/wiki/GIF
        constexpr std::array<unsigned char, 4> expected{ 0x47, 0x49, 0x46, 0x38 };
        internal_files::check_signature<byteme::RawFileReader>(ipath, expected.data(), expected.size(), "GIF");

    } else if (format == "WEBP") {
        auto ipath = prefix + ".webp";
        std::array<unsigned char, 12> observed;
        internal_files::extract_signature(ipath, observed.data(), observed.size());
        constexpr std::array<unsigned char, 4> first4 { 0x52, 0x49, 0x46, 0x46 };
        constexpr std::array<unsigned char, 4> last4 { 0x57, 0x45, 0x42, 0x50 };
        std::array<unsigned char, 4> observed_first, observed_last;
        std::copy_n(observed.begin(), 4, observed_first.begin());
        std::copy_n(observed.begin() + 8, 4, observed_last.begin());
        if (observed_first != first4 || observed_last != last4) {
            throw std::runtime_error("incorrect WEBP file signature for '" + ipath + "'");
        }

    } else {
        throw std::runtime_error("unsupported format '" + format + "'");
    }

    if (options.image_file_strict_check) {
        options.image_file_strict_check(path, metadata, options);
    }
}

}

}

#endif

