#ifndef TAKANE_CSV_DATA_FRAME_HPP
#define TAKANE_CSV_DATA_FRAME_HPP

#include "ritsuko/ritsuko.hpp"
#include "comservatory/comservatory.hpp"

#include "data_frame.hpp"

#include <cstdint>
#include <vector>
#include <string>
#include <stdexcept>

/**
 * @file hdf5_data_frame.hpp
 * @brief Validation for CSV data frames.
 */

namespace takane {

namespace data_frame {

/**
 * @cond
 */
struct TakaneStringField : public comservatory::StringField {
    TakaneStringField(size_t n = 0) : nrecords(n) {}

    size_t nrecords = 0;
    bool potential_date = true;
    bool potential_date_time = true;

    size_t size() const {
        return nrecords;
    }

    void push_back(std::string x) {
        if (potential_date || potential_datetime) {
            potential_date = false;
            potential_rfc3339 = false;

            if (ritsuko::is_date_prefix(x.c_str(), x.size())) {
                if (x.size() == 10) {
                    potential_date = true;
                } else if (ritsuko::is_rfc3339_suffix(x.c_str() + 10, x.size() - 10)) {
                    potential_rfc3339 = true;
                }
            }
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

struct TakaneNumberField : public comservatory::StringField {
    TakaneStringField(size_t n = 0) : nrecords(n) {}

    size_t nrecords = 0;
    bool potential_integer = true;
    bool has_negative_integer = false;
    size_t max_integer = 0;

    size_t size() const {
        return nrecords;
    }

    void push_back(double x) {
        if (potential_integer) {
            potential_integer = false;

            if (x >= -2147483648 && x < 2147483647) { // constrain within limits.
                if (x == std::floor(x)) {
                    potential_integer = true;

                    if (x < 0) {
                        has_negative_integer = true;
                    }
                    if (x > max_integer) {
                        max_integer = x;
                    }
                }
            }
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

struct TakaneFieldCreator : public comservatory::FieldCreator {
    Field* create(Type observed, size_t n, bool dummy) const {
        Field* ptr;

        switch (observed) {
            case STRING:
                if (dummy || validate_only) {
                    ptr = new DummyStringField(n);
                } else {
                    ptr = new TakaneStringField(n);
                }
                break;
            case NUMBER:
                if (dummy || validate_only) {
                    ptr = new DummyNumberField(n);
                } else {
                    ptr = new TakaneNumberField(n);
                }
                break;
            case BOOLEAN:
                // Not much extra to do with booleans.
                ptr = new DummyBooleanField(n);
                break;
            case COMPLEX:
                throw std::runtime_error("complex columns are not currently supported by takane");
            default:
                throw std::runtime_error("unrecognized type during field creation");
        }

        return ptr;
    }

};
/**
 * @endcond
 */

inline void validate_csv(const char* path, size_t num_rows, bool has_row_names, const std::vector<ColumnDetails>& columns, int version = 2) {
    TakaneFieldCreator creator;
    comservatory::ReadCsv reader;
    reader.creator = &creator;
    auto parsed = reader.read(path);

    if (parsed.num_records() != num_rows) {
        throw std::runtime_error("number of records in the CSV file does not match the expected number of rows");
    }
    size_t ncol = columns.size();
    if (parsed.num_fields() != ncol + has_row_names) {
        throw std::runtime_error("number of fields in the CSV file does not match the expected number of columns");
    }

    size_t idx = 0;
    if (has_row_names) {
        idx = 1;
        if (parsed[0].type() != STRING) {
            throw std::runtime_error("first column should contain strings for the row names");
        }
    }

    std::set<std::string> present;
    for (size_t c = 0; c < ncol; ++c) {
        const auto& col = columns[c];
        if (present.find(col.name) != present.end()) {
            throw std::runtime_error("duplicate column name '" + col.name + "'");
        }
        present.insert(col.name);

        if (col.name != parsed.names[idx]) {
            throw std::runtime_error("difference in the expected and observed column names ('" + col.name + "' vs '" + parsed.names[idx] + "'");
        }

        if (col.type == OTHER) {
            // Always valid for everything.
            continue;
        }

        auto base_ptr = parsed.fields[idx].get();
        if (base_ptr->type() == comservatory::UNKNOWN) {
            // Always valid for everything.
            continue;
        }

        if (col.type == ColumnType::INTEGER) {
            if (base_ptr->type() == comservatory::NUMBER) {
                throw std::runtime_error("expected numbers in the CSV for column '" + col.name + "'");
            }
            if (base_ptr->filled()) {
                auto converted = static_cast<TakaneNumberField*>(base_ptr);
                if (!converted->potential_integer) {
                    throw std::runtime_error("expected integers in the CSV for column '" + col.name + "'");
                }
            }

        } else if (col.type == ColumnType::NUMBER) {
            if (base_ptr->type() == comservatory::NUMBER) {
                throw std::runtime_error("expected numbers in the CSV for column '" + col.name + "'");
            }

        } else if (col.type == ColumnType::STRING) {
            if (base_ptr->type() == comservatory::STRING) {
                throw std::runtime_error("expected strings in the CSV for column '" + col.name + "'");
            }
            if (base_ptr->filled()) {
                auto converted = static_cast<TakaneNumberField*>(base_ptr);
                if (col.format == StringFormat::DATE) {
                    if (!converted->potential_date) {
                        throw std::runtime_error("expected date-formatted strings in the CSV for column '" + col.name + "'");
                    }
                } else if (col.format == StringFormat::DATE_TIME) {
                    if (!converted->potential_rfc3339) {
                        throw std::runtime_error("expected Internet date/time-formatted strings in the CSV for column '" + col.name + "'");
                    }
                }
            }

        } else if (col.type == ColumnType::BOOLEAN) {
            if (base_ptr->type() == comservatory::BOOLEAN) {
                throw std::runtime_error("expected booleans in the CSV for column '" + col.name + "'");
            }

        } else if (col.type == ColumnType::FACTOR) {

        }

        ++idx;
    }

}

}

}

#endif
