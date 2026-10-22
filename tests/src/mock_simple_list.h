#ifndef MOCK_SIMPLE_LIST_H
#define MOCK_SIMPLE_LIST_H

#include <filesystem>
#include <string>

#include "byteme/byteme.hpp"
#include "utils.h"

inline void initialize_simple_list_with_metadata(const std::filesystem::path& dir, const std::string& version, const std::string& format) {
    initialize_directory(dir);
    std::ofstream output(dir / "OBJECT");
    output << "{ \"type\": \"simple_list\", \"simple_list\": { \"version\": \"" << version << "\", \"format\": \"" << format << "\" } }";
}

inline void dump_compressed_json(const std::filesystem::path& dir, const std::string& buffer) {
    auto path = dir / "list_contents.json.gz";
    byteme::GzipFileWriter writer(path.c_str(), {});
    writer.write(reinterpret_cast<const unsigned char*>(buffer.data()), buffer.size());
}

inline void mock_simple_list(const std::filesystem::path& dir) {
    initialize_simple_list_with_metadata(dir, "1.0", "json.gz");
    dump_compressed_json(dir, "{ \"type\": \"list\", \"values\": [] }");
}

#endif
