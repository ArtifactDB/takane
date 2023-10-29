#ifndef TAKANE_SEQUENCE_INFORMATION_HPP
#define TAKANE_SEQUENCE_INFORMATION_HPP

#include "ritsuko/ritsuko.hpp"
#include "comservatory/comservatory.hpp"

#include "data_frame.hpp"

#include <unordered_set>
#include <string>
#include <stdexcept>

/**
 * @file sequence_information.hpp
 * @brief Validation for sequence information.
 */

namespace takane {

namespace sequence_information {

/**
 * @brief Summary of the sequence information file.
 */
struct Summary {
    /**
     * Names of all reference sequences in the file.
     * Only filled if `Options::save_seqnames` is true.
     */
    std::vector<std::string> seqnames;
};

/**
 * @brief Parameters for validating the sequence information file.
 */
struct Parameters {
    /**
     * Expected number of sequences.
     */
    size_t num_sequences = 0;

    /**
     * Whether to load and parse the file in parallel, see `comservatory::ReadOptions` for details.
     */
    bool parallel = false;

    /**
     * Whether to save the reference sequence names.
     */
    bool save_seqnames = true;

    /**
     * Version of the `sequence_information` format.
     */
    int version = 1;
};

/**
 * @cond
 */
struct SeqnamesField : public comservatory::DummyStringField {
    SeqnamesField(bool store) : save_names(store) {}

    void add_missing() {
        throw std::runtime_error("missing values should not be present in the seqnames column");
    }

    void push_back(std::string x) {
        if (collected.find(x) != collected.end()) {
            throw std::runtime_error("duplicated sequence name '" + x + "'");
        }
        collected.insert(x);

        if (save_names) {
            ordered.push_back(x);
        }
        comservatory::DummyStringField::push_back(std::move(x));
    }

    std::unordered_set<std::string> collected;
    bool save_names;
    std::vector<std::string> ordered;
};

struct SeqlengthsField : public comservatory::DummyNumberField {
    void push_back(double x) {
        if (x < 0 || x > 2147483647) { // constrain within limits.
            throw std::runtime_error("sequence length must be non-negative and fit inside a 32-bit signed integer");
        }
        if (x != std::floor(x)) {
            throw std::runtime_error("sequence length is not an integer");
        }
        comservatory::DummyNumberField::push_back(x);
    }
};

template<class ParseCommand>
Summary validate_base(ParseCommand parse, const Parameters& params) {
    comservatory::Contents contents;

    contents.names.push_back("seqnames");
    auto ptr = new SeqnamesField(params.save_seqnames);
    contents.fields.emplace_back(ptr);

    contents.names.push_back("seqlengths");
    contents.fields.emplace_back(new SeqlengthsField);
    contents.names.push_back("isCircular");
    contents.fields.emplace_back(new comservatory::DummyBooleanField);
    contents.names.push_back("genome");
    contents.fields.emplace_back(new comservatory::DummyStringField);

    comservatory::ReadOptions opt;
    opt.parallel = params.parallel;
    parse(contents, opt);
    if (contents.num_records() != params.num_sequences) {
        throw std::runtime_error("number of records in the CSV file does not match the expected number of ranges");
    }

    Summary output;
    output.seqnames.swap(ptr->ordered);
    return output;
}
/**
 * @endcond
 */

/**
 * Checks if a CSV data frame is correctly formatted for sequence information.
 * An error is raised if the file does not meet the specifications.
 *
 * @tparam Reader A **byteme** reader class.
 *
 * @param reader A stream of bytes from the CSV file.
 * @param params Validation parameters.
 */
template<class Reader>
Summary validate(Reader& reader, const Parameters& params) {
    return validate_base(
        [&](comservatory::Contents& contents, const comservatory::ReadOptions& opts) -> void { comservatory::read(reader, contents, opts); },
        params
    );
}

/**
 * Overload of `sequence_information::validate()` that accepts a file path.
 *
 * @param path Path to the CSV file.
 * @param params Validation parameters.
 */
inline Summary validate(const char* path, const Parameters& params) {
    return validate_base(
        [&](comservatory::Contents& contents, const comservatory::ReadOptions& opts) -> void { comservatory::read_file(path, contents, opts); },
        params
    );
}

}

}

#endif
