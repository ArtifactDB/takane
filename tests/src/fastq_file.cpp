#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "takane/fastq_file.hpp"
#include "utils.h"

#include <string>
#include <vector>
#include <filesystem>
#include <stdexcept>

struct FastqFileTest : public ::testing::Test {
    FastqFileTest() {
        dir = "TEST_fastq_file";
        name = "fastq_file";
    }

    std::filesystem::path dir;
    std::string name;

    template<typename ... Args_>
    void expect_error(const std::string& msg, Args_&& ... args) {
        expect_validation_error(dir, msg, std::forward<Args_>(args)...);
    }
};

TEST_F(FastqFileTest, Basic) {
    initialize_directory_simple(dir, name, "2.0");
    expect_error("unsupported version");

    initialize_directory(dir);

    auto objpath = (dir / "OBJECT").string();
    quick_text_write(objpath,
        "{ \"type\": \"" + name + "\", \"" + name + "\": { \"version\": \"1.0\", \"sequence_type\": \"DNA\", \"quality_type\": \"phred\", \"quality_offset\": 33 } }"
    );

    auto fqpath = (dir / "file.fastq.gz").string();
    quick_gzip_write(fqpath, "asdasd\nACGT\n+\n!!!!\n");
    expect_error("start with '@'");

    quick_gzip_write(fqpath, "@asdasd\nACGT\n+\n!!!!\n");
    test_validate(dir);

    // Checking the metadata categories.
    quick_text_write(objpath,
        "{ \"type\": \"" + name + "\", \"" + name + "\": { \"version\": \"1.0\" } }"
    );
    expect_error("sequence_type");

    quick_text_write(objpath,
        "{ \"type\": \"" + name + "\", \"" + name + "\": { \"version\": \"1.0\", \"sequence_type\": \"DNA\" } }"
    );
    expect_error("quality_type");

    quick_text_write(objpath,
        "{ \"type\": \"" + name + "\", \"" + name + "\": { \"version\": \"1.0\", \"sequence_type\": \"DNA\", \"quality_type\": true } }"
    );
    expect_error("JSON string");

    quick_text_write(objpath,
        "{ \"type\": \"" + name + "\", \"" + name + "\": { \"version\": \"1.0\", \"sequence_type\": \"DNA\", \"quality_type\": \"foo\" } }"
    );
    expect_error("unknown value 'foo'");

    quick_text_write(objpath,
        "{ \"type\": \"" + name + "\", \"" + name + "\": { \"version\": \"1.0\", \"sequence_type\": \"DNA\", \"quality_type\": \"solexa\" } }"
    );
    test_validate(dir);

    // Checking the quality offset.
    quick_text_write(objpath,
        "{ \"type\": \"" + name + "\", \"" + name + "\": { \"version\": \"1.0\", \"sequence_type\": \"DNA\", \"quality_type\": \"phred\" } }"
    );
    expect_error("quality_offset");

    quick_text_write(objpath,
        "{ \"type\": \"" + name + "\", \"" + name + "\": { \"version\": \"1.0\", \"sequence_type\": \"DNA\", \"quality_type\": \"phred\", \"quality_offset\": true } }"
    );
    expect_error("JSON number");

    quick_text_write(objpath,
        "{ \"type\": \"" + name + "\", \"" + name + "\": { \"version\": \"1.0\", \"sequence_type\": \"DNA\", \"quality_type\": \"phred\", \"quality_offset\": 20 } }"
    );
    expect_error("33 or 64");

    quick_text_write(objpath,
        "{ \"type\": \"" + name + "\", \"" + name + "\": { \"version\": \"1.0\", \"sequence_type\": \"DNA\", \"quality_type\": \"phred\", \"quality_offset\": 64 } }"
    );
    test_validate(dir);
}

TEST_F(FastqFileTest, Indexed) {
    initialize_directory(dir);

    auto objpath = (dir / "OBJECT").string();
    quick_text_write(objpath,
        "{ \"type\": \"" + name + "\", \"" + name + "\": { \"version\": \"1.0\", \"indexed\": true, \"sequence_type\": \"DNA\", \"quality_type\": \"solexa\" } }"
    );

    auto fqpath = (dir / "file.fastq.bgz").string();
    quick_gzip_write(fqpath, "asdasd\nACGT\n+\n!!!!\n");
    expect_error("start with '@'");

    quick_gzip_write(fqpath, "@asdasd\nACGT\n+\n!!!!\n");
    expect_error("missing FASTQ index file");

    quick_gzip_write((dir / "file.fastq.fai").string(), "");
    expect_error("missing BGZF index file");

    quick_gzip_write((dir / "file.fastq.bgz.gzi").string(), "");
    test_validate(dir);
}

TEST_F(FastqFileTest, Strict) {
    initialize_directory(dir);

    auto objpath = (dir / "OBJECT").string();
    quick_text_write(objpath,
        "{ \"type\": \"" + name + "\", \"" + name + "\": { \"version\": \"1.0\", \"sequence_type\": \"DNA\", \"quality_type\": \"phred\", \"quality_offset\": 64 } }"
    );

    auto fqpath = (dir / "file.fastq.gz").string();
    quick_gzip_write(fqpath, "@asdasd\nACGT\n+\n!!!!\n");

    takane::Options opts;
    opts.fastq_file_strict_check = [](const std::filesystem::path&, const takane::ObjectMetadata&, takane::Options&, bool) {};
    test_validate(dir);

    opts.fastq_file_strict_check = [](const std::filesystem::path&, const takane::ObjectMetadata&, takane::Options&, bool) { throw std::runtime_error("ARGH"); };
    expect_error("ARGH", opts);
}
