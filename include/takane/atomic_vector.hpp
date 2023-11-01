#ifndef TAKANE_ATOMIC_VECTOR_HPP
#define TAKANE_ATOMIC_VECTOR_HPP

#include "utils.hpp"
#include "comservatory/comservatory.hpp"

#include <stdexcept>

/**
 * @file atomic_vector.hpp
 * @brief Validation for atomic vectors.
 */

namespace takane {

namespace atomic_vector {

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
 * @brief Parameters for validating the atomic vector file.
 */
struct Parameters {
    /**
     * Length of the atomic vector.
     */
    size_t length = 0;

    /** 
     * Type of the atomic vector.
     */
    Type type = Type::INTEGER;

    /**
     * Whether the vector is named.
     */
    bool has_names = false;

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
 * @cond
 */
template<class ParseCommand>
void validate_base(ParseCommand parse, const Parameters& params) {
    comservatory::Contents contents;
    if (params.has_names) {
        contents.fields.emplace_back(new KnownNameField(false));
    }

    switch(params.type) {
        case Type::INTEGER:
            contents.fields.emplace_back(new KnownIntegerField(static_cast<int>(params.has_names)));
            break;
        case Type::NUMBER:
            contents.fields.emplace_back(new comservatory::DummyNumberField);
            break;
        case Type::STRING:
            contents.fields.emplace_back(new comservatory::DummyStringField);
            break;
        case Type::BOOLEAN:
            contents.fields.emplace_back(new comservatory::DummyBooleanField);
            break;
    }

    comservatory::ReadOptions opt;
    opt.parallel = params.parallel;
    parse(contents, opt);
    if (contents.num_records() != params.length) {
        throw std::runtime_error("number of records in the CSV file does not match the expected length");
    }

    if (contents.names.back() != "values") {
        throw std::runtime_error("column containing vector contents should be named 'values'");
    }
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
 * @param params Validation parameters.
 */
template<class Reader>
void validate(Reader& reader, const Parameters& params) {
    return validate_base(
        [&](comservatory::Contents& contents, const comservatory::ReadOptions& opts) -> void { comservatory::read(reader, contents, opts); },
        params
    );
}

/**
 * Overload of `atomic_vector::validate()` that takes a file path.
 *
 * @param path Path to the CSV file.
 * @param params Validation parameters.
 */
inline void validate(const char* path, const Parameters& params) {
    return validate_base(
        [&](comservatory::Contents& contents, const comservatory::ReadOptions& opts) -> void { comservatory::read_file(path, contents, opts); },
        params
    );
}

}

}

#endif
