#ifndef TAKANE_UTILS_HPP
#define TAKANE_UTILS_HPP

#include "comservatory/comservatory.hpp"

#include <cstdint>

namespace takane {

struct KnownNameField : public comservatory::KnownStringField {
    KnownNameField(bool ar) : as_rownames(ar) {}

    void add_missing() {
        throw std::runtime_error("missing values should not be present in the " + (as_rownames ? std::string("row names") : std::string("names")) + " column");
    }

    bool as_rownames;
};

template<typename T>
constexpr T upper_integer_limit() {
    return 2147483647;
}

template<typename T>
constexpr T lower_integer_limit() {
    return -2147483648;
}

struct KnownIntegerField : public comservatory::DummyNumberField {
    KnownIntegerField(int cid) : column_id(cid) {}

    void push_back(double x) {
        if (x < lower_integer_limit<double>() || x > upper_integer_limit<double>()) { // constrain within limits.
            throw std::runtime_error("value in column " + std::to_string(column_id + 1) + " does not fit inside a 32-bit signed integer");
        }
        if (x != std::floor(x)) {
            throw std::runtime_error("value in column " + std::to_string(column_id + 1) + " is not an integer");
        }
        comservatory::DummyNumberField::push_back(x);
    }

    int column_id;
};

struct KnownNonNegativeIntegerField : public comservatory::DummyNumberField {
    KnownNonNegativeIntegerField(int cid) : column_id(cid) {}

    void push_back(double x) {
        if (x < 0) {
            throw std::runtime_error("value in column " + std::to_string(column_id + 1) + " should not be negative");
        }
        if (x > upper_integer_limit<double>()) {
            throw std::runtime_error("value in column " + std::to_string(column_id + 1) + " does not fit inside a 32-bit signed integer");
        }
        if (x != std::floor(x)) {
            throw std::runtime_error("value in column " + std::to_string(column_id + 1) + " is not an integer");
        }
        comservatory::DummyNumberField::push_back(x);
    }

    int column_id;
};

}

#endif
