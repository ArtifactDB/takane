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
void validate_base(
    ParseCommand parse,
    size_t length,
    size_t concatenated,
    bool has_names,
    const Options& options)
{
    comservatory::Contents contents;
    if (has_names) {
        contents.fields.emplace_back(new KnownNameField(false));
    }

    auto ptr = new KnownCompressedLengthField(static_cast<int>(has_names));
    contents.fields.emplace_back(ptr);

    comservatory::ReadOptions opt;
    opt.parallel = options.parallel;
    parse(contents, opt);
    if (contents.num_records() != length) {
        throw std::runtime_error("number of records in the CSV file does not match the expected length");
    }

    if (concatenated != ptr->total) {
        throw std::runtime_error("sum of lengths in the compressed list did not equal the expected concatenated total");
    }

    if (contents.names.back() != "number") {
        throw std::runtime_error("column containing the compressed list lengths should be named 'number'");
    }
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
 * @param concatenated Total length of the concatenated elements.
 * @param has_names Whether the compressed list is named.
 * @param options Parsing options.
 */
template<class Reader>
void validate(
    Reader& reader,
    size_t length,
    size_t concatenated,
    bool has_names,
    Options options = Options())
{
    validate_base(
        [&](comservatory::Contents& contents, const comservatory::ReadOptions& opts) -> void { comservatory::read(reader, contents, opts); },
        length,
        concatenated,
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
 * @param concatenated Total length of the concatenated elements.
 * @param has_names Whether the compressed list is named.
 * @param options Parsing options.
 */
inline void validate(
    const char* path,
    size_t length,
    size_t concatenated,
    bool has_names,
    Options options = Options())
{
    return validate_base(
        [&](comservatory::Contents& contents, const comservatory::ReadOptions& opts) -> void { comservatory::read_file(path, contents, opts); },
        length,
        concatenated,
        has_names,
        options
    );
}

}

}

#endif
