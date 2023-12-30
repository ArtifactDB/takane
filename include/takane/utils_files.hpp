#ifndef TAKANE_UTILS_FILES_HPP
#define TAKANE_UTILS_FILES_HPP

#include <string>
#include <stdexcept>
#include <filesystem>

#include "utils_other.hpp"
#include "byteme/byteme.hpp"

namespace takane {

namespace internal_files {

inline void check_signature(const std::filesystem::path& path, const char* expected, size_t len, const char* msg) {
    auto reader = internal_other::open_reader<byteme::RawFileReader>(path, /* buffer_size = */ len);
    byteme::PerByte<> pb(&reader);
    bool okay = pb.valid();
    for (size_t i = 0; i < len; ++i) {
        if (!okay) {
            throw std::runtime_error("incomplete " + std::string(msg) + " file signature for '" + path.string() + "'");
        }
        if (pb.get() != expected[i]) {
            throw std::runtime_error("incorrect " + std::string(msg) + " file signature for '" + path.string() + "'");
        }
        okay = pb.advance();
    }
}

inline void check_signature(const std::filesystem::path& path, const unsigned char* expected, size_t len, const char* msg) {
    auto reader = internal_other::open_reader<byteme::RawFileReader>(path, /* buffer_size = */ len);
    byteme::PerByte<unsigned char> pb(&reader);
    bool okay = pb.valid();
    for (size_t i = 0; i < len; ++i) {
        if (!okay) {
            throw std::runtime_error("incomplete " + std::string(msg) + " file signature for '" + path.string() + "'");
        }
        if (pb.get() != expected[i]) {
            throw std::runtime_error("incorrect " + std::string(msg) + " file signature for '" + path.string() + "'");
        }
        okay = pb.advance();
    }
}

inline void extract_signature(const std::filesystem::path& path, unsigned char* store, size_t len) {
    auto reader = internal_other::open_reader<byteme::RawFileReader>(path, /* buffer_size = */ len);
    byteme::PerByte<unsigned char> pb(&reader);
    bool okay = pb.valid();
    for (size_t i = 0; i < len; ++i) {
        if (!okay) {
            throw std::runtime_error("file at '" + path.string() + "' is too small to extract a signature of length " + std::to_string(len));
        }
        store[i] = pb.get();
        okay = pb.advance();
    }
}

}

}

#endif
