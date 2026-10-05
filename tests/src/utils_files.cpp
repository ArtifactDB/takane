#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "takane/utils_files.hpp"

#include "utils.h"

TEST(CheckRawFileSignature, Character) {
    auto dir = define_test_path("utils_files");
    initialize_directory(dir);
    auto path = dir / "foo.png";

    {
        std::ofstream handle(path);
        handle << "";
    }
    expect_error(
        "file is too small",
        [&]() -> void {
            takane::check_raw_file_signature(path, "FOOBAR", 6, "an ASD file");
        }
    );

    {
        std::ofstream handle(path);
        handle << "FOObar";
    }
    expect_error(
        "incorrect signature",
        [&]() -> void {
            takane::check_raw_file_signature(path, "FOOBAR", 6, "an ASD file");
        }
    );

    {
        std::ofstream handle(path);
        handle << "FOOBAR";
    }
    takane::check_raw_file_signature(path, "FOOBAR", 6, "an ASD file");


    // Works with non-ASCII characters.
    {
        std::ofstream handle(path);
        handle << "FOO\1BAR\2asdasd\3asd\n";
    }
    takane::check_raw_file_signature(path, "FOO\1BAR\2", 8, "an ASD file");
}

TEST(CheckRawFileSignature, Unsigned) {
    auto dir = define_test_path("utils_files");
    initialize_directory(dir);
    auto path = (dir / "foo.bam").string();
    const unsigned char foo[4] = { 0x4a, 0x55, 0xf2, 0x90 };

    {
        std::ofstream handle(path);
        handle << "";
    }
    expect_error(
        "file is too small",
        [&]() -> void {
            takane::check_raw_file_signature(path, foo, 4, "an ASD file");
        }
    );

    {
        std::ofstream handle(path);
        handle << "FOObar";
    }
    expect_error(
        "incorrect signature",
        [&]() -> void {
            takane::check_raw_file_signature(path, foo, 4, "an ASD file");
        }
    );

    {
        byteme::RawFileWriter writer(path.c_str(), {});
        writer.write(foo, 4);
    }
    takane::check_raw_file_signature(path, foo, 4, "an ASD file");
}

TEST(CheckGunzippedFileSignature, Character) {
    auto dir = define_test_path("utils_files");
    initialize_directory(dir);
    auto path = dir / "whee.png";

    {
        quick_gzip_write(path, "");
    }
    expect_error(
        "file is too small",
        [&]() -> void {
            takane::check_gunzipped_file_signature(path, "FOOBAR", 6, "an ASD file");
        }
    );

    {
        quick_gzip_write(path, "asdaasdasd");
    }
    expect_error(
        "incorrect signature",
        [&]() -> void {
            takane::check_gunzipped_file_signature(path, "FOOBAR", 6, "an ASD file");
        }
    );

    {
        quick_gzip_write(path, "FOOBAR123");
    }
    takane::check_gunzipped_file_signature(path, "FOOBAR", 6, "an ASD file");
}

TEST(CheckGunzippedFileSignature, Unsigned) {
    auto dir = define_test_path("utils_files");
    initialize_directory(dir);
    auto path = dir / "whee.txt.gz";
    const unsigned char foo[4] = { 0x4a, 0x55, 0xf2, 0x90 };

    {
        byteme::GzipFileWriter writer(path.c_str(), {});
    }
    expect_error(
        "file is too small",
        [&]() -> void {
            takane::check_gunzipped_file_signature(path, foo, 4, "an ASD file");
        }
    );

    {
        byteme::GzipFileWriter writer(path.c_str(), {});
        std::string msg = "asdaasdasd";
        writer.write(reinterpret_cast<const unsigned char*>(msg.c_str()), msg.size());
    }
    expect_error(
        "incorrect signature",
        [&]() -> void {
            takane::check_gunzipped_file_signature(path, foo, 4, "an ASD file");
        }
    );

    {
        byteme::GzipFileWriter writer(path.c_str(), {});
        writer.write(foo, 4);
    }
    takane::check_gunzipped_file_signature(path, foo, 4, "an ASD file");
}

TEST(CheckGzipFileSignature, Basic) {
    auto dir = define_test_path("utils_files");
    initialize_directory(dir);
    auto path = dir / "whee.txt.gz";

    {
        byteme::RawFileWriter writer(path.c_str(), {});
    }
    expect_error(
        "file is too small",
        [&]() -> void {
            takane::check_gzip_file_signature(path);
        }
    );

    {
        byteme::GzipFileWriter writer(path.c_str(), {});
    }
    takane::check_gzip_file_signature(path);
}

TEST(ExtractRawFileSignature, Basic) {
    auto dir = define_test_path("utils_files");
    initialize_directory(dir);
    auto path = dir / "foo.bam";

    unsigned char buffer[4];
    {
        std::ofstream handle(path);
        handle << "";
    }
    expect_error(
        "too small",
        [&]() -> void {
            takane::extract_raw_file_signature(path, buffer, 4, /* must_work = */ true);
        }
    );

    {
        std::ofstream handle(path);
        handle << "FOObar";
    }
    {
        EXPECT_EQ(takane::extract_raw_file_signature(path, buffer, 4, /* must_work = */ true), 4);
        auto cbuffer = reinterpret_cast<char*>(buffer);
        EXPECT_EQ(cbuffer[0], 'F');
        EXPECT_EQ(cbuffer[1], 'O');
        EXPECT_EQ(cbuffer[2], 'O');
        EXPECT_EQ(cbuffer[3], 'b');
    }

    {
        std::ofstream handle(path);
        handle << "bar";
    }
    {
        EXPECT_EQ(takane::extract_raw_file_signature(path, buffer, 4, /* must_work = */ false), 3);
        auto cbuffer = reinterpret_cast<char*>(buffer);
        EXPECT_EQ(cbuffer[0], 'b');
        EXPECT_EQ(cbuffer[1], 'a');
        EXPECT_EQ(cbuffer[2], 'r');
    }
}

TEST(ExtractGunzippedFileSignature, Basic) {
    auto dir = define_test_path("utils_files");
    initialize_directory(dir);
    auto path = dir / "foo.bam";

    unsigned char buffer[4];
    {
        quick_gzip_write(path, "");
    }
    expect_error(
        "too small",
        [&]() -> void {
            takane::extract_gunzipped_file_signature(path, buffer, 4, /* must_work = */ true);
        }
    );

    {
        quick_gzip_write(path, "FOObar");
    }
    {
        EXPECT_EQ(takane::extract_gunzipped_file_signature(path, buffer, 4, /* must_work = */ true), 4);
        auto cbuffer = reinterpret_cast<char*>(buffer);
        EXPECT_EQ(cbuffer[0], 'F');
        EXPECT_EQ(cbuffer[1], 'O');
        EXPECT_EQ(cbuffer[2], 'O');
        EXPECT_EQ(cbuffer[3], 'b');
    }

    {
        quick_gzip_write(path, "bar");
    }
    {
        EXPECT_EQ(takane::extract_gunzipped_file_signature(path, buffer, 4, /* must_work = */ false), 3);
        auto cbuffer = reinterpret_cast<char*>(buffer);
        EXPECT_EQ(cbuffer[0], 'b');
        EXPECT_EQ(cbuffer[1], 'a');
        EXPECT_EQ(cbuffer[2], 'r');
    }
}


TEST(IsFileIndexed, Basic) {
    takane::JsonObjectMap obj;
    EXPECT_FALSE(takane::is_file_indexed(obj));

    obj["indexed"] = std::shared_ptr<millijson::Base>(new millijson::Number(100));
    expect_error(
        "JSON boolean",
        [&]() -> void {
            takane::is_file_indexed(obj);
        }
    );

    obj["indexed"] = std::shared_ptr<millijson::Base>(new millijson::Boolean(false));
    EXPECT_FALSE(takane::is_file_indexed(obj));

    obj["indexed"] = std::shared_ptr<millijson::Base>(new millijson::Boolean(true));
    EXPECT_TRUE(takane::is_file_indexed(obj));
}

TEST(ValdiateSequenceType, Basic) {
    takane::JsonObjectMap obj;
    expect_error(
        "expected a 'sequence_type' property",
        [&]() -> void {
            takane::validate_sequence_type(obj);
        }
    );

    obj["sequence_type"] = std::shared_ptr<millijson::Base>(new millijson::Number(100));
    expect_error(
        "should be a JSON string",
        [&]() -> void {
            takane::validate_sequence_type(obj);
        }
    );

    obj["sequence_type"] = std::shared_ptr<millijson::Base>(new millijson::String("whee"));
    expect_error(
        "unsupported value",
        [&]() -> void {
            takane::validate_sequence_type(obj);
        }
    );

    obj["sequence_type"] = std::shared_ptr<millijson::Base>(new millijson::String("custom"));
    takane::validate_sequence_type(obj);
}
