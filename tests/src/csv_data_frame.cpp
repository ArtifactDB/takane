#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "takane/csv_data_frame.hpp"

#include <numeric>
#include <string>
#include <vector>
#include <fstream>

static void validate(const std::string& buffer, size_t num_rows, bool has_row_names, const std::vector<takane::data_frame::ColumnDetails>& columns, int df_version = 2) {
    std::string path = "TEST-csv_data_frame.csv";
    {
        std::ofstream ohandle(path);
        ohandle << buffer;
    }

    takane::csv_data_frame::Parameters opt;
    opt.num_rows = num_rows;
    opt.has_row_names = has_row_names;
    opt.columns = &columns;
    opt.df_version = df_version;

    takane::csv_data_frame::validate(path.c_str(), opt);
}

template<typename ... Args_>
static void expect_error(const std::string& msg, const std::string& buffer, Args_&& ... args) {
    EXPECT_ANY_THROW({
        try {
            validate(buffer, std::forward<Args_>(args)...);
        } catch (std::exception& e) {
            EXPECT_THAT(e.what(), ::testing::HasSubstr(msg));
            throw;
        }
    });
}

TEST(CsvDataFrame, Rownames) {
    std::vector<takane::data_frame::ColumnDetails> columns(1);
    columns[0].name = "aaron";
    std::string buffer = "\"rows\",\"aaron\"\n\"mizuki\",54\n\"fumika\",21\n\"aiko\",55\n";
    validate(buffer, 3, true, columns);

    {
        std::string buffer = "\"rows\",\"aaron\"\n\"mizuki\",54\nNA,21\n\"aiko\",55\n";
        expect_error("missing values should not be present", buffer, 3, true, columns);
    }
}

TEST(CsvDataFrame, General) {
    std::vector<takane::data_frame::ColumnDetails> columns(2);
    columns[0].name = "Aaron";
    columns[0].type = takane::data_frame::ColumnType::STRING;
    columns[1].name = "Barry";
    columns[1].type = takane::data_frame::ColumnType::NUMBER;

    std::string buffer = "\"Aaron\",\"Barry\"\n\"mizuki\",54\n\"fumika\",21\n\"aiko\",55\n";
    validate(buffer, 3, false, columns);
    expect_error("number of records", buffer, 50, false, columns);

    {
        std::string buffer = "\"Aaron\",\"Barry\",\"Charlie\"\n\"mizuki\",54,true\n\"fumika\",21,false\n\"aiko\",55,true\n";
        expect_error("provided number of fields", buffer, 3, false, columns);

        columns.resize(columns.size() + 1);
        columns.back().name = "Charlie";
        columns.back().type = takane::data_frame::ColumnType::BOOLEAN;
        validate(buffer, 3, false, columns);
        columns.pop_back();
    }

    {
        columns.back().type = takane::data_frame::ColumnType::BOOLEAN;
        expect_error("types do not match",buffer, 3, false, columns);
        columns.back().type = takane::data_frame::ColumnType::NUMBER;
    }

    {
        columns[1].name = "FOO";
        expect_error("header names do not match", buffer, 3, false, columns);
        columns[1].name = "Aaron";
        expect_error("duplicate column name", buffer, 3, false, columns);
        columns[1].name = "Barry";
    }

    {
        columns[0].type = takane::data_frame::ColumnType::OTHER;
        validate(buffer, 3, false, columns);
        columns[1].type = takane::data_frame::ColumnType::OTHER;
        validate(buffer, 3, false, columns);
        columns[0].type = takane::data_frame::ColumnType::STRING;
        columns[1].type = takane::data_frame::ColumnType::NUMBER;
    }
}

TEST(CsvDataFrame, Integer) {
    std::vector<takane::data_frame::ColumnDetails> columns(1);
    columns[0].name = "Barry";
    columns[0].type = takane::data_frame::ColumnType::INTEGER;

    {
        std::string buffer = "\"Aaron\",\"Barry\"\n\"mizuki\",54\n\"fumika\",21\n\"aiko\",55\n";
        validate(buffer, 3, true, columns);
    }

    {
        std::string buffer = "\"Aaron\",\"Barry\"\n\"mizuki\",54.2\n\"fumika\",21\n\"aiko\",55\n";
        expect_error("is not an integer", buffer, 3, true, columns);
    }

    {
        std::string buffer = "\"Aaron\",\"Barry\"\n\"mizuki\",54\n\"fumika\",2147483648\n\"aiko\",55\n";
        expect_error("32-bit signed integer", buffer, 3, true, columns);
    }
}

TEST(CsvDataFrame, StringDate) {
    std::vector<takane::data_frame::ColumnDetails> columns(1);
    columns[0].name = "Charlie";
    columns[0].type = takane::data_frame::ColumnType::STRING;
    columns[0].format = takane::data_frame::StringFormat::DATE;

    {
        std::string buffer = "\"Aaron\",\"Charlie\"\n\"mio\",\"2019-12-31\"\n\"rin\",\"1991-07-31\"\n\"uzuki\",\"1967-01-23\"\n";
        validate(buffer, 3, true, columns);
    }

    {
        std::string buffer = "\"Aaron\",\"Charlie\"\n\"mio\",\"2019-12-31\"\n\"rin\",\"whee\"\n\"uzuki\",\"1967-01-23\"\n";
        expect_error("date in column", buffer, 3, true, columns);
    }
}

TEST(CsvDataFrame, StringDateTime) {
    std::vector<takane::data_frame::ColumnDetails> columns(1);
    columns[0].name = "Omega";
    columns[0].type = takane::data_frame::ColumnType::STRING;
    columns[0].format = takane::data_frame::StringFormat::DATE_TIME;

    {
        std::string buffer = "\"rows\",\"Omega\"\n\"mio\",\"2019-12-31T01:34:21+21:00\"\n\"rin\",\"1991-07-31T15:56:02.1Z\"\n\"uzuki\",\"1967-01-23T21:33:42.324-09:30\"\n";
        validate(buffer, 3, true, columns);
    }

    {
        std::string buffer = "\"rows\",\"Omega\"\n\"mio\",\"2019-12-31T01:34:21+21:00\"\n\"rin\",\"whee\"\n\"uzuki\",\"1967-01-23T21:33:42.324-09:30\"\n";
        expect_error("expected an Internet date/time", buffer, 3, true, columns);
    }
}

TEST(CsvDataFrame, FactorVersion1) {
    std::vector<takane::data_frame::ColumnDetails> columns(1);
    columns[0].name = "Omega";
    columns[0].type = takane::data_frame::ColumnType::FACTOR;
    columns[0].factor_levels = std::unordered_set<std::string>{ "hiori", "mano", "meguru", "kiriko" };

    {
        std::string buffer = "\"Omega\"\n\"hiori\"\n\"meguru\"\n\"hiori\"\n\"kiriko\"\n\"mano\"\n\"meguru\"\n";
        validate(buffer, 6, false, columns, /* version = */ 1);
    }

    {
        std::string buffer = "\"Omega\"\n\"hiori\"\n\"meguru\"\n\"hiori\"\n\"yay\"\n\"mano\"\n\"meguru\"\n";
        expect_error("does not refer to a valid level", buffer, 6, false, columns, /* version = */ 1);
    }
}

TEST(CsvDataFrame, FactorVersion2) {
    std::vector<takane::data_frame::ColumnDetails> columns(1);
    columns[0].name = "Omega";
    columns[0].type = takane::data_frame::ColumnType::FACTOR;
    columns[0].factor_levels = std::unordered_set<std::string>{ "hiori", "mano", "meguru", "kiriko" };

    {
        std::string buffer = "\"Omega\"\n2\n1\n3\n0\n0\n1\n2\n2\n";
        validate(buffer, 8, false, columns);
    }

    {
        std::string buffer = "\"Omega\"\n2\n1\n3\n-1\n0\n1\n2\n2\n";
        expect_error("non-negative", buffer, 8, false, columns);
        buffer = "\"Omega\"\n2\n1\n3\n1\n5\n1\n2\n2\n";
        expect_error("less than the number of levels", buffer, 8, false, columns);
        buffer = "\"Omega\"\n2\n1.1\n3\n1\n5\n1\n2\n2\n";
        expect_error("is not an integer", buffer, 8, false, columns);
    }
}

TEST(CsvDataFrame, FromReader) {
    std::string buffer = "\"rows\",\"aaron\"\n\"mizuki\",54\n\"fumika\",21\n\"aiko\",55\n";
    byteme::RawBufferReader reader(reinterpret_cast<const unsigned char*>(buffer.c_str()), buffer.size());

    takane::csv_data_frame::Parameters params;
    params.num_rows = 3;
    params.has_row_names = true;
    auto& columns = params.columns.mutable_ref();
    columns.resize(1);
    columns[0].name = "aaron";

    takane::csv_data_frame::validate(reader, params);
}
