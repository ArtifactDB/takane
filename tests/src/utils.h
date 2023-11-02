#ifndef UTILS_H
#define UTILS_H

#include "takane/utils_csv.hpp"

struct FilledFieldCreator : public takane::CsvFieldCreator {
    comservatory::StringField* string() {
        return new comservatory::FilledStringField;
    }

    comservatory::NumberField* number() {
        return new comservatory::FilledNumberField;
    }

    comservatory::BooleanField* boolean() {
        return new comservatory::FilledBooleanField;
    }
};

#endif
