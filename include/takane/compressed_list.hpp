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
 * @brief Parameters for validating the compressed list file.
 */
struct Parameters {
    /**
     * Length of the compressed list.
     */
    size_t length = 0;
 
    /**
     * Total length of the concatenated elements.
     */
    size_t concatenated = 0;

    /**
     * Whether the compressed list is named.
     */
    bool has_names = false;

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

    void add_missing() {
        throw std::runtime_error("lengths should not be missing");
    }

    void push_back(double x) {
        KnownNonNegativeIntegerField::push_back(x);
        total += static_cast<size_t>(x);
    }

    size_t total = 0;
};

template<class ParseCommand>
void validate_base(ParseCommand parse, const Parameters& params) {
    comservatory::Contents contents;
    if (params.has_names) {
        contents.fields.emplace_back(new KnownNameField(false));
    }

    auto ptr = new KnownCompressedLengthField(static_cast<int>(params.has_names));
    contents.fields.emplace_back(ptr);

    comservatory::ReadOptions opt;
    opt.parallel = params.parallel;
    parse(contents, opt);
    if (contents.num_records() != params.length) {
        throw std::runtime_error("number of records in the CSV file does not match the expected length");
    }

    if (params.concatenated != ptr->total) {
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
 * Overload of `compressed_list::validate()` that accepts a file path.
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
