#ifndef TAKANE_DATA_FRAME_HPP
#define TAKANE_DATA_FRAME_HPP

namespace takane {

namespace data_frame {

enum class ColumnType {
    INTEGER,
    NUMBER,
    STRING,
    BOOLEAN,
    FACTOR,
    OTHER
};

enum class StringFormat {
    NONE,
    DATE,
    DATE_TIME
};

struct ColumnDetails {
    std::string name;
    ColumnType type = ColumnType::INTEGER;
    StringFormat format = StringFormat::NONE;
    int32_t factor_levels = 0;
};

}

}

#endif
