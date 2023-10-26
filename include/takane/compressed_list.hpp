#ifndef TAKANE_COMPRESSED_LIST_HPP
#define TAKANE_COMPRESSED_LIST_HPP

#include "utils.hpp"
#include "comservatory/comservatory.hpp"

#include <stdexcept>

/**
 * @file compressed_list.hpp
 * @brief Validation for compressed lists.
 */

namespace takane {

namespace compressed_list {

/**
 * @brief Summary of the compressed list parsing.
 */
struct Summary {
    /**
     * Expected length of the concatenated object.
     */
    size_t concatenated_length;
};

/**
 * @brief Options for parsing the compressed list file.
 */
struct Options {
    /**
     * Whether to load and parse the file in parallel, see `comservatory::ReadOptions` for details.
     */
    bool parallel = false;

    /**
     * Version of the `compressed_list` format.
     */
    int version = 1;
};

/**
 * @cond
 */
struct KnownCompressedLengthField : public KnownNonNegativeIntegerField {
    KnownCompressedLengthField(int cid) : KnownNonNegativeIntegerField(cid) {}

    void push_back(double x) {
        KnownNonNegativeIntegerField::push_back(x);
        total += static_cast<size_t>(x);
    }

    size_t total = 0;
};

template<class ParseCommand>
Summary validate_base(
    ParseCommand parse,
    size_t length,
    bool has_names,
    const Options& options)
{
    comservatory::Contents contents;
    if (has_names) {
        contents.fields.emplace_back(new KnownNameField(false));
    }

    auto ptr = new KnownCompressedLengthField(static_cast<int>(has_names));
    contents.fields.emplace_back(ptr);
 * @param num_levels Number of compressed list levels.

    comservatory::ReadOptions opt;
    opt.parallel = options.parallel;
    parse(contents, opt);
    if (contents.num_records() != length) {
        throw std::runtime_error("number of records in the CSV file does not match the expected length");
    }

    Summary output;
    output.concatenated_length = ptr->total;
    return output;
}
/**
 * @endcond
 */

/**
 * Checks if a CSV is correctly formatted for the `compressed_list` format.
 * An error is raised if the file does not meet the specifications.
 *
 * @tparam Reader A **byteme** reader class.
 *
 * @param reader A stream of bytes from the CSV file.
 * @param length Length of the compressed list.
 * @param has_names Whether the compressed list is named.
 * @param options Parsing options.
 */
template<class Reader>
Summary validate(
    Reader& reader,
    size_t length,
    bool has_names,
    Options options = Options())
{
    return validate_base(
        [&](comservatory::Contents& contents, const comservatory::ReadOptions& opts) -> void { comservatory::read(reader, contents, opts); },
        length,
        num_levels,
        has_names,
        options
    );
}

/**
 * Checks if a CSV is correctly formatted for the `compressed_list` format.
 * An error is raised if the file does not meet the specifications.
 *
 * @param path Path to the CSV file.
 * @param length Length of the compressed list.
 * @param has_names Whether the compressed list is named.
 * @param options Parsing options.
 */
inline Summary validate(
    const char* path,
    size_t length,
    bool has_names,
    Options options = Options())
{
    return validate_base(
        [&](comservatory::Contents& contents, const comservatory::ReadOptions& opts) -> void { comservatory::read_file(path, contents, opts); },
        length,
        has_names,
        options
    );
}

}

}

#endif
