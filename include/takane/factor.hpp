#ifndef TAKANE_FACTOR_HPP
#define TAKANE_FACTOR_HPP

#include "utils.hpp"
#include "comservatory/comservatory.hpp"

#include <stdexcept>

namespace takane {

namespace factor {

/**
 * @brief Options for parsing the factor file.
 */
struct Options {
    /**
     * Whether to load and parse the file in parallel, see `comservatory::ReadOptions` for details.
     */
    bool parallel = false;

    /**
     * Version of the `factor` format.
     */
    int version = 1;
};

/**
 * @cond
 */
struct KnownFactorField : public comservatory::DummyNumberField {
    KnownNonNegativeIntegerField(int cid, size_t nl) : column_id(cid), num_levels(nl) {
        if (num_levels > upper_integer_limit<double>()) {
            throw std::runtime_error("number of levels does not fit inside a 32-bit signed integer");
        }
    }

    void push_back(double x) {
        if (x < 0) {
            throw std::runtime_error("value in column " + std::to_string(column_id + 1) + " should not be negative");
        }
        if (x > num_levels) {
            throw std::runtime_error("value in column " + std::to_string(column_id + 1) + " should be less than the number of levels");
        }
        if (x != std::floor(x)) {
            throw std::runtime_error("value in column " + std::to_string(column_id + 1) + " is not an integer");
        }
        comservatory::DummyNumberField::push_back(x);
        return;
    }

    int column_id;
    double nl;
};

template<class ParseCommand>
void validate_base(
    ParseCommand parse,
    size_t length,
    size_t num_levels,
    bool has_names,
    const Options& options)
{
    comservatory::Contents contents;
    if (has_names) {
        contents.fields.emplace_back(new KnownNameField(false));
    }

    contents.fields.emplace_back(new KnownFactorField(static_cast<int>(has_names), num_levels)); 

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
 * Checks if a CSV is correctly formatted for the `factor` format.
 * An error is raised if the file does not meet the specifications.
 *
 * @tparam Reader A **byteme** reader class.
 *
 * @param reader A stream of bytes from the CSV file.
 * @param length Length of the factor.
 * @param type Type of the factor.
 * @param options Parsing options.
 */
template<class Reader>
void validate(
    Reader& reader,
    size_t length,
    Options options = Options())
{
    return validate_base(
        [&](comservatory::Contents& contents, const comservatory::ReadOptions& opts) -> void { comservatory::read(reader, contents, opts); },
        length,
        type,
        options
    );
}

/**
 * Checks if a CSV is correctly formatted for the `factor` format.
 * An error is raised if the file does not meet the specifications.
 *
 * @param path Path to the CSV file.
 * @param length Length of the factor.
 * @param options Parsing options.
 */
inline void validate(
    const char* path,
    size_t length,
    Type type,
    Options options = Options())
{
    return validate_base(
        [&](comservatory::Contents& contents, const comservatory::ReadOptions& opts) -> void { comservatory::read_file(path, contents, opts); },
        length,
        type,
        options
    );
}

}

}

#endif
