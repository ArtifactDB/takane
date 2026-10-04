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
 * @cond
 */
// Factored out for re-use in spatial_experiment::internal::validate_image.
inline void validate_png_image(const std::filesystem::path& path) {
    // Magic number from http://www.libpng.org/pub/png/spec/1.2/png-1.2-pdg.html#PNG-file-signature
    constexpr std::array<unsigned char, 8> expected { 137, 80, 78, 71, 13, 10, 26, 10 };
    check_raw_file_signature(path, expected.data(), expected.size(), "a PNG file");
}

inline void validate_tiff_image(const std::filesystem::path& path) {
    std::array<unsigned char, 4> observed{};
    extract_file_signature(path, observed.data(), observed.size());
    // Magic numbers from https://en.wikipedia.org/wiki/Magic_number_(programming)
    constexpr std::array<unsigned char, 4> iisig { 0x49, 0x49, 0x2A, 0x00 };
    constexpr std::array<unsigned char, 4> mmsig { 0x4D, 0x4D, 0x00, 0x2A };
    if (observed != iisig && observed != mmsig) {
        throw std::runtime_error("incorrect TIFF file signature");
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
inline void validate_image_file(const std::filesystem::path& path, const ObjectMetadata& metadata, const Options& options) {
    const std::string type_name = "image_file"; // use a separate variable to avoid dangling reference warnings from GCC.

    const std::string* format;
    try {
        const auto& obj = extract_json_object(metadata.other, type_name);
        try {
            const auto& vstring = extract_json_version_string(obj);
            auto version = ritsuko::parse_version_string(vstring.c_str(), vstring.size(), /* skip_patch = */ true);
            if (version.major != 1) {
                throw std::runtime_error("unsupported version string '" + vstring + "'");
            }
        } catch (...) {
            std::throw_with_nested(std::runtime_error("failed to validate 'version'"));
        }

        try {
            const std::string format_name = "format"; // again, avoid dangling refs.
            format = &extract_json_string(obj, format_name);
        } catch (...) {
            std::throw_with_nested(std::runtime_error("failed to read 'format'"));
        }
    } catch (...) {
        std::throw_with_nested(std::runtime_error("failed to validate '" + type_name + "' in the object metadata"));
    }

    if (*format == "PNG") {
        const auto ipath = path / "file.png";
        try {
            validate_png_image(ipath);
        } catch (...) {
            std::throw_with_nested(std::runtime_error("failed to validate '" + ipath.filename().string() + "'"));
        }

    } else if (*format == "TIFF") {
        const auto ipath = path / "file.tif";
        try {
            validate_tiff_image(ipath);
        } catch (...) {
            std::throw_with_nested(std::runtime_error("failed to validate '" + ipath.filename().string() + "'"));
        }

    } else if (*format == "JPEG") {
        const auto ipath = path / "file.jpg";
        try {
            // Common prefix of the JPEG-related magic numbers from https://en.wikipedia.org/wiki/List_of_file_signatures
            constexpr std::array<unsigned char, 2> expected { 0xFF, 0xD8 };
            check_raw_file_signature(ipath, expected.data(), expected.size(), "a JPEG file");
        } catch (...) {
            std::throw_with_nested(std::runtime_error("failed to validate '" + ipath.filename().string() + "'"));
        }

    } else if (*format == "GIF") {
        const auto ipath = path / "file.gif";
        try {
            // Common prefix of the old and new magic numbers from https://en.wikipedia.org/wiki/GIF
            constexpr std::array<unsigned char, 4> expected{ 0x47, 0x49, 0x46, 0x38 };
            check_raw_file_signature(ipath, expected.data(), expected.size(), "a GIF file");
        } catch (...) {
            std::throw_with_nested(std::runtime_error("failed to validate '" + ipath.filename().string() + "'"));
        }

    } else if (*format == "WEBP") {
        const auto ipath = path / "file.webp";
        try {
            std::array<unsigned char, 12> observed;
            extract_file_signature(ipath, observed.data(), observed.size());
            constexpr std::array<unsigned char, 4> first4 { 0x52, 0x49, 0x46, 0x46 };
            constexpr std::array<unsigned char, 4> last4 { 0x57, 0x45, 0x42, 0x50 };
            std::array<unsigned char, 4> observed_first, observed_last;
            std::copy_n(observed.begin(), 4, observed_first.begin());
            std::copy_n(observed.begin() + 8, 4, observed_last.begin());
            if (observed_first != first4 || observed_last != last4) {
                throw std::runtime_error("incorrect WEBP file signature");
            }
        } catch (...) {
            std::throw_with_nested(std::runtime_error("failed to validate '" + ipath.filename().string() + "'"));
        }

    } else {
        throw std::runtime_error("unsupported format '" + *format + "' in '/" + type_name + "/format' from the object metadata");
    }

    if (options.image_file_strict_check) {
        options.image_file_strict_check(path, metadata, options);
    }
}

}

#endif

