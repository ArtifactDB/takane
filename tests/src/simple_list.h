#ifndef SIMPLE_LIST_H
#define SIMPLE_LIST_H

#include <filesystem>
#include <string>

#include "byteme/byteme.hpp"

namespace simple_list {

inline void dump_compressed_json(const std::filesystem::path& dir, const std::string& buffer) {
    auto path = dir / "list_contents.json.gz";
    byteme::GzipFileWriter writer(path.c_str());
    writer.write(reinterpret_cast<const unsigned char*>(buffer.data()), buffer.size());
}

inline void mock(const std::filesystem::path& dir) {
    dump_compressed_json(dir, "{ \"type\": \"list\", \"values\": [] }");
}

}

#endif
