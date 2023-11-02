#ifndef TAKANE_UTILS_CSV_HPP
#define TAKANE_UTILS_CSV_HPP

#include "comservatory/comservatory.hpp"

namespace takane {

struct FieldCreator {
    ~FieldCreator() = default;

    virtual comservatory::NumberField* integer() = 0;

    virtual comservatory::StringField* string() = 0;

    virtual comservatory::NumberField* number() = 0;

    virtual comservatory::BooleanField* boolean() = 0;
};

struct DummyFieldCreator : public FieldCreator {
    comservatory::NumberField* integer() {
        return new comservatory::DummyNumberField;
    }

    comservatory::StringField* string() {
        return new comservatory::DummyStringField;
    }

    comservatory::NumberField* number() {
        return new comservatory::DummyNumberField;
    }

    comservatory::BooleanField* boolean() {
        return new comservatory::DummyBooleanField;
    }
};

/**
 * @cond
 */
struct KnownNameField : public comservatory::StringField {
    KnownNameField(bool ar, comservatory::StringField* p) : as_rownames(ar), child(p) {}

public:
    bool as_rownames;
    comservatory::StringField* child;

public:
    void add_missing() {
        throw std::runtime_error("missing values should not be present in the " + (as_rownames ? std::string("row names") : std::string("names")) + " column");
    }

    void push_back(std::string x) {
        child->push_back(std::move(x));
    }

    size_t size() const {
        return child->size();
    }

    bool filled() const {
        return true;
    }
};

template<typename T>
constexpr T upper_integer_limit() {
    return 2147483647;
}

template<typename T>
constexpr T lower_integer_limit() {
    return -2147483648;
}

struct KnownIntegerField : public comservatory::NumberField {
    KnownIntegerField(int cid, comservatory::NumberField* p) : column_id(cid), child(p) {}

public:
    int column_id;
    comservatory::NumberField* child;

public:
    void add_missing() {
        child->add_missing();
    }

    void push_back(double x) {
        if (x < lower_integer_limit<double>() || x > upper_integer_limit<double>()) { // constrain within limits.
            throw std::runtime_error("value in column " + std::to_string(column_id + 1) + " does not fit inside a 32-bit signed integer");
        }
        if (x != std::floor(x)) {
            throw std::runtime_error("value in column " + std::to_string(column_id + 1) + " is not an integer");
        }
        child->push_back(x);
    }

    size_t size() const {
        return child->size();
    }

    bool filled() const {
        return true;
    }
};

struct KnownNonNegativeIntegerField : public comservatory::NumberField {
    KnownNonNegativeIntegerField(int cid, comservatory::NumberField* p) : column_id(cid), child(p) {}

public:
    int column_id;
    comservatory::NumberField* child;

public:
    void add_missing() {
        child->add_missing();
    }

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
        child->push_back(x);
    }

    size_t size() const {
        return child->size();
    }

    bool filled() const {
        return true;
    }
};

struct KnownDateField : public comservatory::StringField {
    KnownDateField(int cid, comservatory::StringField* p) : column_id(cid), child(p) {}

public:
    int column_id;
    comservatory::StringField* child;

public:
    void push_back(std::string x) {
        if (!ritsuko::is_date(x.c_str(), x.size())) {
            throw std::runtime_error("expected a date in column " + std::to_string(column_id + 1) + ", got '" + x + "' instead");
        }
        child->push_back(std::move(x));
    }

    void add_missing() {
        child->add_missing();
    }

    size_t size() const {
        return child->size();
    }

    bool filled() const { 
        return true;
    }
};

struct KnownDateTimeField : public comservatory::StringField {
    KnownDateTimeField(int cid, comservatory::StringField* p) : column_id(cid), child(p) {}

public:
    int column_id;
    comservatory::StringField* child;

public:
    void push_back(std::string x) {
        if (!ritsuko::is_rfc3339(x.c_str(), x.size())) {
            throw std::runtime_error("expected an Internet date/time in column " + std::to_string(column_id + 1) + ", got '" + x + "' instead");
        }
        child->push_back(std::move(x));
    }

    void add_missing() {
        child->add_missing();
    }

    size_t size() const {
        return child->size();
    }

    bool filled() const { 
        return true;
    }
};

struct FactorV1Field : public comservatory::StringField {
    FactorV1Field(int cid, const std::unordered_set<std::string>* l, comservatory::StringField* p) : column_id(cid), levels(l), child(p) {}

public:
    int column_id;
    const std::unordered_set<std::string>* levels;
    comservatory::StringField* child;

public:
    void push_back(std::string x) {
        if (levels->find(x) == levels->end()) {
            throw std::runtime_error("value in column " + std::to_string(column_id + 1) + " does not refer to a valid level");
        }
        child->push_back(std::move(x));
    }

    void add_missing() {
        child->add_missing();
    }

    size_t size() const {
        return child->size();
    }

    bool filled() const { 
        return true;
    }
};

struct FactorV2Field : public comservatory::NumberField {
    FactorV2Field(int cid, size_t nlevels, comservatory::NumberField* p) : column_id(id), nlevels(l), child(p) {
        if (nlevels > upper_integer_limit<size_t>()) {
            throw std::runtime_error("number of levels must fit into a 32-bit signed integer");
        }
    }

public:
    int column_id;
    double nlevels; // casting for an easier comparison.
    comservatory::NumberField* child;

public:
    void push_back(double x) {
        if (x < 0 || x >= nlevels) {
            throw std::runtime_error("value in column " + std::to_string(column_id + 1) + " does not refer to a valid level");
        }
        if (x != std::floor(x)) {
            throw std::runtime_error("value in column " + std::to_string(column_id + 1) + " is not an integer");
        }
        child->push_back(x);
    }

    void add_missing() {
        child->add_missing();
    }

    size_t size() const {
        return child->size();
    }

    bool filled() const { 
        return true;
    }
};
/**
 * @endcond
 */

}

#endif
