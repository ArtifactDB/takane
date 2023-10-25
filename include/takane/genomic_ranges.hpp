#ifndef TAKANE_GENOMIC_RANGES_HPP
#define TAKANE_GENOMIC_RANGES_HPP

#include "ritsuko/ritsuko.hpp"
#include "comservatory/comservatory.hpp"

#include "data_frame.hpp"

#include <unordered_set>
#include <string>
#include <stdexcept>

namespace takane {

namespace genomic_ranges {

/**
 * @cond
 */
struct NamesField : public comservatory::DummyStringField {
    void add_missing() {
        throw std::runtime_error("missing values should not be present in the names column");
    }
};

struct SeqnamesField : public comservatory::DummyStringField {
    void add_missing() {
        throw std::runtime_error("missing values should not be present in the seqnames column");
    }

    void push_back(std::string x) {
        if (all_seqnames->find(x) == all_seqnames->end()) {
            throw std::runtime_error("unknown sequence name '" + x + "'");
        }
        comservatory::DummyStringField::push_back(std::move(x));
    }

    const std::unordered_set<std::string>* all_seqnames = NULL;
};

struct StartField : public comservatory::DummyNumberField {
    void add_missing() {
        throw std::runtime_error("missing values should not be present in the start column");
    }

    void push_back(double x) {
        if (x < -2147483648 || x > 2147483647) { // constrain within limits.
            throw std::runtime_error("start position does not fit inside a 32-bit signed integer");
        }
        if (x != std::floor(x)) {
            throw std::runtime_error("start position is not an integer");
        }
        last = x;
        comservatory::DummyNumberField::push_back(x);
    }

    int32_t last = 0;
};

struct EndField : public comservatory::DummyNumberField {
    void add_missing() {
        throw std::runtime_error("missing values should not be present in the strand column");
    }

    void push_back(double x) {
        if (x < -2147483648 || x > 2147483647) { // constrain within limits.
            throw std::runtime_error("end position does not fit inside a 32-bit signed integer");
        }
        if (x != std::floor(x)) {
            throw std::runtime_error("end position is not an integer");
        }
        comservatory::DummyNumberField::push_back(x);

        if (start->size() != size()) {
            throw std::runtime_error("'start' and 'end' validator fields are out of sync");
        }
        if (x + 1 < start->last) {
            throw std::runtime_error("'end' coordinate must be greater than or equal to 'start - 1'");
        }
    }

    const StartField* start = NULL;
};

struct StrandField : public comservatory::DummyStringField {
    void add_missing() {
        throw std::runtime_error("missing values should not be present in the strand column");
    }

    void push_back(std::string x) {
        if (x.size() != 1 || (x[0] != '+' && x[0] != '-' && x[0] != '*')) {
            throw std::runtime_error("invalid strand '" + x + "'");
        }
        comservatory::DummyStringField::push_back(std::move(x));
    }
};

template<class ParseCommand>
void validate_base(
    ParseCommand parse, 
    size_t num_ranges, 
    bool has_names, 
    const std::unordered_set<std::string>& seqnames, 
    int /* version */ = 1)
{
    comservatory::Contents contents;
    if (has_names) {
        contents.fields.emplace_back(new NamesField);
    }

    {
        auto ptr = new SeqnamesField;
        ptr->all_seqnames = &seqnames;
        contents.fields.emplace_back(ptr);
    }
        
    {
        auto sptr = new StartField;
        contents.fields.emplace_back(sptr);
        auto eptr = new EndField;
        eptr->start = sptr;
        contents.fields.emplace_back(eptr);
    }

    contents.fields.emplace_back(new StrandField);

    parse(contents);
    if (contents.num_records() != num_ranges) {
        throw std::runtime_error("number of records in the CSV file does not match the expected number of ranges");
    }

    if (contents.names[0 + has_names] != "seqnames") {
        throw std::runtime_error("expected the first (non-name) column to be 'seqnames'");
    }
    if (contents.names[1 + has_names] != "start") {
        throw std::runtime_error("expected the second (non-name) column to be 'start'");
    }
    if (contents.names[2 + has_names] != "end") {
        throw std::runtime_error("expected the third (non-name) column to be 'end'");
    }
    if (contents.names[3 + has_names] != "strand") {
        throw std::runtime_error("expected the fourth (non-name) column to be 'strand'");
    }
}
/**
 * @endcond
 */

/**
 * Checks if a CSV data frame is correctly formatted for genomic ranges.
 * An error is raised if the file does not meet the specifications.
 *
 * @tparam Reader A **byteme** reader class.
 *
 * @param reader A stream of bytes from the CSV file.
 * @param num_ranges Number of genomic ranges in this object.
 * @param has_ranges Whether the ranges are named.
 * @param seqnames Universe of sequence names for this object.
 * @param options Reading options.
 * @param version Version of the `genomic_ranges` format.
 */
template<class Reader>
void validate(
    Reader& reader, 
    size_t num_ranges, 
    bool has_names, 
    const std::unordered_set<std::string>& seqnames, 
    const comservatory::ReadOptions& options,
    int version = 1)
{
    validate_base(
        [&](comservatory::Contents& contents) -> void { comservatory::read(reader, contents, options); },
        num_ranges,
        has_names,
        seqnames,
        version
    );
}

/**
 * Checks if a CSV data frame is correctly formatted for genomic ranges.
 * An error is raised if the file does not meet the specifications.
 *
 * @param path Path to the CSV file.
 * @param num_ranges Number of genomic ranges in this object.
 * @param has_ranges Whether the ranges are named.
 * @param seqnames Universe of sequence names for this object.
 * @param options Reading options.
 * @param df_version Version of the `genomic_ranges` format.
 */
inline void validate(
    const char* path, 
    size_t num_ranges, 
    bool has_names, 
    const std::unordered_set<std::string>& seqnames, 
    const comservatory::ReadOptions& options,
    int version = 1)
{
    validate_base(
        [&](comservatory::Contents& contents) -> void { comservatory::read_file(path, contents, options); },
        num_ranges,
        has_names,
        seqnames,
        version
    );
}

}

}

#endif