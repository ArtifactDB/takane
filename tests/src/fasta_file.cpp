#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "takane/fasta_file.hpp"
#include "utils.h"

#include <string>
#include <vector>
#include <filesystem>
#include <stdexcept>

struct FastaFileTest : public ::testing::Test {
    FastaFileTest() {
        dir = "TEST_fasta_file";
        name = "fasta_file";
    }

    std::filesystem::path dir;
    std::string name;

    template<typename ... Args_>
    void expect_error(const std::string& msg, Args_&& ... args) {
        expect_validation_error(dir, msg, std::forward<Args_>(args)...);
    }
};

TEST_F(FastaFileTest, Basic) {
    initialize_directory_simple(dir, name, "2.0");
    expect_error("unsupported version");

    initialize_directory(dir);

    auto objpath = (dir / "OBJECT").string();
    quick_text_write(objpath,
        "{ \"type\": \"" + name + "\", \"" + name + "\": { \"version\": \"1.0\", \"sequence_type\": \"DNA\" } }"
    );

    auto fapath = (dir / "file.fasta.gz").string();
    quick_gzip_write(fapath, "asdasd\nACGT\n");
    expect_error("start with '>'");

    quick_gzip_write(fapath, ">asdasd\nACGT\n");
    test_validate(dir);

    // Works with different sequence types.
    quick_text_write(objpath,
        "{ \"type\": \"" + name + "\", \"" + name + "\": { \"version\": \"1.0\", \"sequence_type\": \"RNA\" } }"
    );
    test_validate(dir);

    quick_text_write(objpath,
        "{ \"type\": \"" + name + "\", \"" + name + "\": { \"version\": \"1.0\", \"sequence_type\": \"AA\" } }"
    );
    test_validate(dir);

    quick_text_write(objpath,
        "{ \"type\": \"" + name + "\", \"" + name + "\": { \"version\": \"1.0\", \"sequence_type\": \"custom\" } }"
    );
    test_validate(dir);

    quick_text_write(objpath,
        "{ \"type\": \"" + name + "\", \"" + name + "\": { \"version\": \"1.0\", \"sequence_type\": \"foo\" } }"
    );
    expect_error("foo");
}

TEST_F(FastaFileTest, Indexed) {
    initialize_directory(dir);

    quick_text_write((dir / "OBJECT").string(),
        "{ \"type\": \"" + name + "\", \"" + name + "\": { \"version\": \"1.0\", \"indexed\": true, \"sequence_type\": \"DNA\" } }"
    );

    auto fbpath = (dir / "file.fasta.bgz").string();
    quick_gzip_write(fbpath, "asdasd\nACGT\n");
    expect_error("start with '>'");

    quick_gzip_write(fbpath, ">asdasd\nACGT\n");
    expect_error("missing FASTA index file");

    quick_text_write((dir / "file.fasta.fai").string(), "");
    expect_error("missing BGZF index file");

    quick_text_write((dir / "file.fasta.bgz.gzi").string(), "");
    test_validate(dir);
}

TEST_F(FastaFileTest, Strict) {
    initialize_directory(dir);

    quick_text_write((dir / "OBJECT").string(),
        "{ \"type\": \"" + name + "\", \"" + name + "\": { \"version\": \"1.0\", \"sequence_type\": \"DNA\" } }"
    );
    quick_gzip_write((dir / "file.fasta.gz").string(), ">asdasd\nACGT\n");

    takane::Options opts;
    opts.fasta_file_strict_check = [](const std::filesystem::path&, const takane::ObjectMetadata&, takane::Options&, bool) {};
    test_validate(dir);

    opts.fasta_file_strict_check = [](const std::filesystem::path&, const takane::ObjectMetadata&, takane::Options&, bool) { throw std::runtime_error("ARGH"); };
    expect_error("ARGH", opts);
}
