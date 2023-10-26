#ifndef TAKANE_ATOMIC_VECTOR_HPP
#define TAKANE_ATOMIC_VECTOR_HPP

#include "utils.hpp"
#include "comservatory/comservatory.hpp"

#include <stdexcept>

namespace takane {

namespace atomic_vector {

/**
 * @brief Options for parsing the string vector file.
 */
struct Options {
    /**
     * Whether to load and parse the file in parallel, see `comservatory::ReadOptions` for details.
     */
    bool parallel = false;

    /**
     * Version of the `atomic_vector` format.
     */
    int version = 1;
};

/**
 * Type of the atomic vector.
 *
 * - `INTEGER`: a number that can be represented by a 32-bit signed integer.
 * - `NUMBER`: a number that can be represented by a double-precision float.
 * - `STRING`: a string.
 * - `BOOLEAN`: a boolean.
 */
enum class Type {
    INTEGER,
    NUMBER,
    STRING,
    BOOLEAN
};

/**
 * @cond
 */
template<class ParseCommand>
void validate_base(
    ParseCommand parse,
    size_t length,
    Type type,
    bool has_names,
    const Options& options)
{
    comservatory::Contents contents;
    if (has_names) {
        contents.fields.emplace_back(new KnownNameField(false));
    }

    switch(type) {
        case INTEGER:
            contents.fields.emplace_back(new KnownIntegerField(has_names));
            break;
        case NUMBER:
            contents.fields.emplace_back(new comservatory::DummyNumberField);
            break;
        case STRING:
            contents.fields.emplace_back(new comservatory::DummyStringField);
            break;
        case Boolean:
            contents.fields.emplace_back(new comservatory::DummyBooleanField);
            break;
    }

    comservatory::ReadOptions opt;
    opt.parallel = options.parallel;
    parse(contents, opt);
    if (contents.num_records() != length) {
        throw std::runtime_error("number of records in the CSV file does not match the expected length");
    }

    return;
}
/**
 * @endcond
 */

/**
 * Checks if a CSV is correctly formatted for the `atomic_vector` format.
 * An error is raised if the file does not meet the specifications.
 *
 * @tparam Reader A **byteme** reader class.
 *
 * @param reader A stream of bytes from the CSV file.
 * @param length Length of the atomic vector.
 * @param type Type of the atomic vector.
 * @param options Parsing options.
 */
template<class Reader>
void validate(
    Reader& reader,
    size_t length,
    Type type,
    bool has_names,
    Options options = Options())
{
    return validate_base(
        [&](comservatory::Contents& contents, const comservatory::ReadOptions& opts) -> void { comservatory::read(reader, contents, opts); },
        length,
        type,
        has_names,
        options
    );
}

/**
 * Checks if a CSV is correctly formatted for the `atomic_vector` format.
 * An error is raised if the file does not meet the specifications.
 *
 * @param path Path to the CSV file.
 * @param length Length of the atomic vector.
 * @param type Type of the atomic vector.
 * @param options Parsing options.
 */
inline void validate(
    const char* path,
    size_t length,
    Type type,
    bool has_names,
    Options options = Options())
{
    return validate_base(
        [&](comservatory::Contents& contents, const comservatory::ReadOptions& opts) -> void { comservatory::read_file(path, contents, opts); },
        length,
        type,
        has_names,
        options
    );
}

}

}

#endif
