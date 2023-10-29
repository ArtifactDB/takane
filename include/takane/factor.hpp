#ifndef TAKANE_FACTOR_HPP
#define TAKANE_FACTOR_HPP

#include "utils.hpp"
#include "comservatory/comservatory.hpp"

#include <stdexcept>

/**
 * @file factor.hpp
 * @brief Validation for factors.
 */

namespace takane {

namespace factor {

/**
 * @brief Parameters for validating the factor file.
 */
struct Parameters {
    /**
     * Length of the factor.
     */
    size_t length = 0;

    /**
     * Number of levels.
     */
    size_t num_levels = 0;

    /**
     * Whether the factor has names.
     */
    bool has_names = false;

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
    KnownFactorField(int cid, size_t nl) : column_id(cid), num_levels(nl) {
        if (num_levels > upper_integer_limit<double>()) {
            throw std::runtime_error("number of levels does not fit inside a 32-bit signed integer");
        }
    }

    void push_back(double x) {
        if (x < 0) {
            throw std::runtime_error("value in column " + std::to_string(column_id + 1) + " should not be negative");
        }
        if (x >= num_levels) {
            throw std::runtime_error("value in column " + std::to_string(column_id + 1) + " should be less than the number of levels");
        }
        if (x != std::floor(x)) {
            throw std::runtime_error("value in column " + std::to_string(column_id + 1) + " is not an integer");
        }
        comservatory::DummyNumberField::push_back(x);
    }

    int column_id;
    double num_levels;
};

template<class ParseCommand>
void validate_base(ParseCommand parse, const Parameters& params) {
    comservatory::Contents contents;
    if (params.has_names) {
        contents.fields.emplace_back(new KnownNameField(false));
    }

    contents.fields.emplace_back(new KnownFactorField(static_cast<int>(params.has_names), params.num_levels)); 

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
 * Checks if a CSV is correctly formatted for the `factor` format.
 * An error is raised if the file does not meet the specifications.
 *
 * @tparam Reader A **byteme** reader class.
 *
 * @param reader A stream of bytes from the CSV file.
 * @param params Validation parameters.
 */
template<class Reader>
void validate(Reader& reader, const Parameters& params) {
    validate_base(
        [&](comservatory::Contents& contents, const comservatory::ReadOptions& opts) -> void { comservatory::read(reader, contents, opts); },
        params
    );
}

/**
 * Overload of `factor::validate()` that accepts a file path.
 *
 * @param path Path to the CSV file.
 * @param params Validation parameters.
 */
inline void validate(const char* path, const Parameters& params) {
    validate_base(
        [&](comservatory::Contents& contents, const comservatory::ReadOptions& opts) -> void { comservatory::read_file(path, contents, opts); },
        params
    );
}

}

}

#endif
