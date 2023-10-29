#ifndef TAKANE_STRING_FACTOR_HPP
#define TAKANE_STRING_FACTOR_HPP

#include "utils.hpp"
#include "comservatory/comservatory.hpp"

#include <stdexcept>
#include <string>
#include <unordered_set>

/**
 * @file string_factor.hpp
 * @brief Validation for string factors.
 */

namespace takane {

namespace string_factor {

/**
 * @brief Parameters for validating the level file.
 */
struct LevelParameters {
    /**
     * Expected number of levels.
     */
    size_t num_levels = 0;

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
struct KnownLevelField : public comservatory::DummyStringField {
    void push_back(std::string x) {
        if (levels.find(x) != levels.end()) {
            throw std::runtime_error("detected a duplicated level '" + x + "' in column 0");
        }
        levels.insert(x);
        comservatory::DummyStringField::push_back(std::move(x));
    }

    std::unordered_set<std::string> levels;
};

template<class ParseCommand>
void validate_levels_base(ParseCommand parse, const LevelParameters& params) {
    comservatory::Contents contents;
    contents.fields.emplace_back(new KnownLevelField);
    contents.names.push_back("values");

    comservatory::ReadOptions opt;
    opt.parallel = params.parallel;
    parse(contents, opt);
    if (contents.num_records() != params.num_levels) {
        throw std::runtime_error("number of records in the CSV file does not match the expected length");
    }
}
/**
 * @endcond
 */

/**
 * Checks if a CSV is correctly formatted for the levels in the `string_factor` format.
 * An error is raised if the file does not meet the specifications.
 *
 * @tparam Reader A **byteme** reader class.
 *
 * @param reader A stream of bytes from the CSV file.
 * @param params Validation parameters.
 */
template<class Reader>
void validate_levels(Reader& reader, const LevelParameters& params) {
    validate_levels_base(
        [&](comservatory::Contents& contents, const comservatory::ReadOptions& opts) -> void { comservatory::read(reader, contents, opts); },
        params
    );
}

/**
 * Overload of `string_factor::validate_levels()` that accepts a file path.
 *
 * @param path Path to the CSV file.
 * @param params Validation parameters.
 */
inline void validate_levels(const char* path, const LevelParameters& params) {
    validate_levels_base(
        [&](comservatory::Contents& contents, const comservatory::ReadOptions& opts) -> void { comservatory::read_file(path, contents, opts); },
        params
    );
}

}

}

#endif
