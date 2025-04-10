#ifndef SIMPLE_LIST_H
#define SIMPLE_LIST_H

#include <filesystem>
#include <string>

#include "byteme/byteme.hpp"
#include "utils.h"

namespace simple_list {

inline void initialize_with_metadata(const std::filesystem::path& dir, const std::string& version, const std::string& format) {
    initialize_directory(dir);
    std::ofstream output(dir / "OBJECT");
    output << "{ \"type\": \"simple_list\", \"simple_list\": { \"version\": \"" << version << "\", \"format\": \"" << format << "\" } }";
}

inline void dump_compressed_json(const std::filesystem::path& dir, const std::string& buffer) {
    auto path = dir / "list_contents.json.gz";
    byteme::GzipFileWriter writer(path.c_str(), {});
    writer.write(reinterpret_cast<const unsigned char*>(buffer.data()), buffer.size());
}

inline void mock(const std::filesystem::path& dir) {
    initialize_with_metadata(dir, "1.0", "json.gz");
    dump_compressed_json(dir, "{ \"type\": \"list\", \"values\": [] }");
}

}

#endif
