#ifndef TAKANE_UTILS_FILES_HPP
#define TAKANE_UTILS_FILES_HPP

#include <string>
#include <stdexcept>
#include <filesystem>
#include <array>
#include <vector>
#include <cstddef>

#include "utils_other.hpp"
#include "utils_json.hpp"
#include "byteme/byteme.hpp"

namespace takane {

template<class Reader_, typename Type_>
void check_file_signature(Reader_& reader, const Type_* expected, std::size_t len, const char* msg) {
    std::vector<Type_> buffer(len);
    if (reader.read(reinterpret_cast<unsigned char*>(buffer.data()), len) != len) {
        throw std::runtime_error("file is too small to extract a signature for " + std::string(msg));
    }
    for (std::size_t i = 0; i < len; ++i) {
        if (buffer[i] != expected[i]) {
            throw std::runtime_error("incorrect file signature for " + std::string(msg));
        }
    }
}

template<typename Type_>
void check_raw_file_signature(const std::filesystem::path& path, const Type_* expected, std::size_t len, const char* msg) {
    auto reader = open_reader<byteme::RawFileReader>(path, byteme::RawFileReaderOptions());
    check_file_signature(*reader, expected, len, msg);
}

template<typename Type_>
void check_gunzipped_file_signature(const std::filesystem::path& path, const Type_* expected, std::size_t len, const char* msg) {
    auto reader = open_reader<byteme::GzipFileReader>(path, byteme::GzipFileReaderOptions());
    check_file_signature(*reader, expected, len, msg);
}

inline void check_gzip_file_signature(const std::filesystem::path& path) {
    std::array<unsigned char, 2> gzmagic { 0x1f, 0x8b };
    check_raw_file_signature(path, gzmagic.data(), gzmagic.size(), "GZIP");
}

inline void extract_file_signature(const std::filesystem::path& path, unsigned char* store, std::size_t len) {
    auto reader = open_reader<byteme::RawFileReader>(path, byteme::RawFileReaderOptions());
    if (reader->read(store, len) != len) {
        throw std::runtime_error("file is too small to extract a signature of length " + std::to_string(len));
    }
}

inline bool is_file_indexed(const JsonObjectMap& objmap) {
    auto iIt = objmap.find("indexed");
    if (iIt == objmap.end()) {
        return false;
    }

    const auto& val = iIt->second;
    if (val->type() != millijson::BOOLEAN) {
        throw std::runtime_error("'indexed' property should be a JSON boolean");
    }

    return reinterpret_cast<const millijson::Boolean*>(val.get())->value();
}

inline void validate_sequence_type(const JsonObjectMap& objmap) {
    auto sIt = objmap.find("sequence_type");
    if (sIt == objmap.end()) {
        throw std::runtime_error("expected a 'sequence_type' property");
    }

    const auto& val = sIt->second;
    if (val->type() != millijson::STRING) {
        throw std::runtime_error("'sequence_type' property should be a JSON string");
    }

    const auto& stype = reinterpret_cast<const millijson::String*>(val.get())->value();
    if (stype != "RNA" && stype != "DNA" && stype != "AA" && stype != "custom") {
        throw std::runtime_error("unsupported value '" + stype + "' for the 'sequence_type' property");
    }
}

}

#endif
