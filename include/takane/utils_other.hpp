#ifndef TAKANE_UTILS_OTHER_HPP
#define TAKANE_UTILS_OTHER_HPP

#include <filesystem>
#include <string>
#include <type_traits>
#include <vector>

#include "sanisizer/sanisizer.hpp"
#include "byteme/byteme.hpp"

#include "utils_public.hpp"

namespace takane {

void validate(const std::filesystem::path&, const ObjectMetadata&, Options&);
size_t height(const std::filesystem::path&, const ObjectMetadata&, Options&);
bool satisfies_interface(const std::string&, const std::string&, const Options&);

template<typename Input_>
using I = std::remove_cv_t<std::remove_reference_t<Input_> >;

template<typename Type_, class Stream_, class Action_>
void iterate_stream(Stream_& stream, Action_ action) {
    auto buffer = sanisizer::create<std::vector<Type_> >(stream.chunk_size());
    while (true) {
        auto available = stream.load(buffer.data());
        if (available == 0) {
            break;
        }
        for (I<decltype(available)> i = 0; i < available; ++i) {
            action(i + stream.start(), std::move(buffer[i]));
        }
    }
}

template<class Reader_, typename Path_, typename ... Args_>
std::unique_ptr<byteme::Reader> open_reader(const Path_& path, Args_&& ... args) {
    if constexpr(std::is_same<typename Path_::value_type, char>::value) {
        return std::make_unique<Reader_>(path.c_str(), std::forward<Args_>(args)...);
    } else {
        // Dealing with windows...
        auto str = path.string();
        return std::make_unique<Reader_>(str.c_str(), std::forward<Args_>(args)...);
    }
}

template<typename Type_>
std::unique_ptr<byteme::BufferedReader<Type_> > wrap_reader_for_bytes(std::unique_ptr<byteme::Reader> reader, bool parallel) {
    if (parallel) {
        return std::make_unique<byteme::ParallelBufferedReader<Type_, decltype(reader)> >(std::move(reader), 65536);
    } else {
        return std::make_unique<byteme::SerialBufferedReader<Type_, decltype(reader)> >(std::move(reader), 65536);
    }
}

inline void validate_mcols(const std::filesystem::path& parent, const std::string& name, size_t expected, Options& options) try {
    auto path = parent / name;
    if (!std::filesystem::exists(path)) {
        return;
    }

    auto xmeta = read_object_metadata(path);
    if (!satisfies_interface(xmeta.type, "DATA_FRAME", options)) {
        throw std::runtime_error("expected an object that satisfies the 'DATA_FRAME' interface");
    }
    ::takane::validate(path, xmeta, options);

    if (::takane::height(path, xmeta, options) != expected) {
        throw std::runtime_error("unexpected number of rows");
    }
} catch (std::exception& e) {
    throw std::runtime_error("failed to validate '" + name + "'; " + std::string(e.what()));
}

inline void validate_metadata(const std::filesystem::path& parent, const std::string& name, Options& options) try {
    auto path = parent / name;
    if (!std::filesystem::exists(path)) {
        return;
    }

    auto xmeta = read_object_metadata(path);
    if (!satisfies_interface(xmeta.type, "SIMPLE_LIST", options)) {
        throw std::runtime_error("expected an object that satisfies the 'SIMPLE_LIST' interface'");
    }
    ::takane::validate(path, xmeta, options);
} catch (std::exception& e) {
    throw std::runtime_error("failed to validate '" + name + "'; " + std::string(e.what()));
}

inline std::size_t count_directory_entries(const std::filesystem::path& path) {
    std::size_t num_dir_obj = 0;
    for (const auto& entry : std::filesystem::directory_iterator(path)) {
        const auto& p = entry.path().filename().string();
        if (p.size() && (p[0] == '.' || p[0] == '_')) {
            continue;
        }
        num_dir_obj = sanisizer::sum<std::size_t>(num_dir_obj, 1);
    }
    return num_dir_obj;
}

}

#endif
