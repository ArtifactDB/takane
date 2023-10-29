#ifndef TAKANE_CSV_DATA_FRAME_HPP
#define TAKANE_CSV_DATA_FRAME_HPP

#include "ritsuko/ritsuko.hpp"
#include "comservatory/comservatory.hpp"

#include "WrappedOption.hpp"
#include "data_frame.hpp"

#include <unordered_set>
#include <string>
#include <stdexcept>

/**
 * @file csv_data_frame.hpp
 * @brief Validation for CSV data frames.
 */

namespace takane {

namespace csv_data_frame {

/**
 * @brief Parameters for validating the CSV data frame.
 */
struct Parameters {
    /**
     * Number of rows in the data frame.
     */
    size_t num_rows = 0;

    /**
     * Whether the data frame contains row names.
     */
    bool has_row_names = false;

    /**
     * Details about the expected columns of the data frame, in order.
     */
    WrappedOption<std::vector<data_frame::ColumnDetails> > columns;
 
    /**
     * Whether to load and parse the file in parallel, see `comservatory::ReadOptions` for details.
     */
    bool parallel = false;

    /**
     * Version of the `data_frame` format.
     */
    int df_version = 2;
};

/**
 * @cond
 */
struct TakaneRowNamesField : public comservatory::DummyStringField {
    void add_missing() {
        throw std::runtime_error("missing values should not be present in the row names column");
    }
};

struct TakaneDateField : public comservatory::StringField {
    TakaneDateField(size_t n = 0, int id = 0) : nrecords(n), column_id(id) {}

    size_t nrecords = 0;
    int column_id = 0;

    size_t size() const {
        return nrecords;
    }

    void push_back(std::string x) {
        if (!ritsuko::is_date(x.c_str(), x.size())) {
            throw std::runtime_error("expected a date in column " + std::to_string(column_id + 1) + ", got '" + x + "' instead");
        }
        ++nrecords;
        return;
    }

    void add_missing() {
        ++nrecords;
        return;
    }

    bool filled() const { 
        return false;
    }
};

struct TakaneDateTimeField : public comservatory::StringField {
    TakaneDateTimeField(size_t n = 0, int id = 0) : nrecords(n), column_id(id) {}

    size_t nrecords = 0;
    int column_id = 0;

    size_t size() const {
        return nrecords;
    }

    void push_back(std::string x) {
        if (!ritsuko::is_rfc3339(x.c_str(), x.size())) {
            throw std::runtime_error("expected an Internet date/time in column " + std::to_string(column_id + 1) + ", got '" + x + "' instead");
        }
        ++nrecords;
        return;
    }

    void add_missing() {
        ++nrecords;
        return;
    }

    bool filled() const { 
        return false;
    }
};

struct TakaneIntegerField : public comservatory::NumberField {
    TakaneIntegerField(size_t n = 0, int id = 0) : nrecords(n), column_id(id) {}

    size_t nrecords = 0;
    int column_id = 0;

    size_t size() const {
        return nrecords;
    }

    void push_back(double x) {
        if (x < -2147483648 || x > 2147483647) { // constrain within limits.
            throw std::runtime_error("value in column " + std::to_string(column_id + 1) + " does not fit inside a 32-bit signed integer");
        }
        if (x != std::floor(x)) {
            throw std::runtime_error("value in column " + std::to_string(column_id + 1) + " is not an integer");
        }
        ++nrecords;
        return;
    }

    void add_missing() {
        ++nrecords;
        return;
    }

    bool filled() const { 
        return false;
    }
};

struct TakaneFactorV1Field : public comservatory::StringField {
    TakaneFactorV1Field(size_t n = 0, int id = 0, const std::unordered_set<std::string>* l = NULL) : nrecords(n), column_id(id), levels(l) {}

    size_t nrecords = 0;
    int column_id = 0;
    const std::unordered_set<std::string>* levels = NULL;

    size_t size() const {
        return nrecords;
    }

    void push_back(std::string x) {
        if (levels->find(x) == levels->end()) {
            throw std::runtime_error("value in column " + std::to_string(column_id + 1) + " does not refer to a valid level");
        }
        ++nrecords;
        return;
    }

    void add_missing() {
        ++nrecords;
        return;
    }

    bool filled() const { 
        return false;
    }
};

struct TakaneFactorV2Field : public comservatory::NumberField {
    TakaneFactorV2Field(size_t n = 0, int id = 0, int l = 0) : nrecords(n), column_id(id), nlevels(l) {
        if (nlevels > 2147483647) {
            throw std::runtime_error("number of levels must fit into a 32-bit signed integer");
        }
    }

    size_t nrecords = 0;
    int column_id = 0;
    double nlevels = 0; // casting for an easier comparison.

    size_t size() const {
        return nrecords;
    }

    void push_back(double x) {
        if (x < 0 || x >= nlevels) {
            throw std::runtime_error("value in column " + std::to_string(column_id + 1) + " does not refer to a valid level");
        }
        if (x != std::floor(x)) {
            throw std::runtime_error("value in column " + std::to_string(column_id + 1) + " is not an integer");
        }
        ++nrecords;
        return;
    }

    void add_missing() {
        ++nrecords;
        return;
    }

    bool filled() const { 
        return false;
    }
};

template<class ParseCommand>
void validate_base(ParseCommand parse, const Parameters& params) {
    comservatory::Contents contents;
    if (params.has_row_names) {
        contents.fields.emplace_back(new TakaneRowNamesField);
    }

    const auto& columns = *(params.columns);
    size_t ncol = columns.size();
    std::unordered_set<std::string> present;
    for (size_t c = 0; c < ncol; ++c) {
        const auto& col = columns[c];
        if (present.find(col.name) != present.end()) {
            throw std::runtime_error("duplicate column name '" + col.name + "'");
        }
        present.insert(col.name);

        if (col.type == data_frame::ColumnType::INTEGER) {
            contents.fields.emplace_back(new TakaneIntegerField(0, c));

        } else if (col.type == data_frame::ColumnType::NUMBER) {
            contents.fields.emplace_back(new comservatory::DummyNumberField);

        } else if (col.type == data_frame::ColumnType::STRING) {
            if (col.format == data_frame::StringFormat::DATE) {
                contents.fields.emplace_back(new TakaneDateField(0, c));
            } else if (col.format == data_frame::StringFormat::DATE_TIME) {
                contents.fields.emplace_back(new TakaneDateTimeField(0, c));
            } else {
                contents.fields.emplace_back(new comservatory::DummyStringField);
            }

        } else if (col.type == data_frame::ColumnType::BOOLEAN) {
            contents.fields.emplace_back(new comservatory::DummyBooleanField);

        } else if (col.type == data_frame::ColumnType::FACTOR) {
            if (params.df_version == 1) {
                contents.fields.emplace_back(new TakaneFactorV1Field(0, c, col.factor_levels.get()));
            } else {
                contents.fields.emplace_back(new TakaneFactorV2Field(0, c, col.factor_levels->size()));
            }
        } else if (col.type == data_frame::ColumnType::OTHER) {
            contents.fields.emplace_back(new comservatory::UnknownField); // This can be anything.

        } else {
            throw std::runtime_error("unknown code for the expected column type");
        }
    }

    comservatory::ReadOptions opt;
    opt.parallel = params.parallel;
    parse(contents, opt);
    if (contents.num_records() != params.num_rows) {
        throw std::runtime_error("number of records in the CSV file does not match the expected number of rows");
    }

    for (size_t c = 0; c < ncol; ++c) {
        const auto& col = columns[c];
        if (col.name != contents.names[c + params.has_row_names]) {
            throw std::runtime_error("observed and expected header names do not match");
        }
    }
}
/**
 * @endcond
 */

/**
 * Checks if a CSV data frame is correctly formatted.
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
        [&](comservatory::Contents& contents, const comservatory::ReadOptions& opt) -> void { comservatory::read(reader, contents, opt); },
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
    validate_base(
        [&](comservatory::Contents& contents, const comservatory::ReadOptions& opt) -> void { comservatory::read_file(path, contents, opt); },
        params
    );
}

}

}

#endif
