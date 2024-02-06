#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "takane/bigbed_file.hpp"
#include "utils.h"

#include <string>
#include <vector>
#include <filesystem>
#include <stdexcept>

struct BigBedFileTest : public ::testing::Test {
    BigBedFileTest() {
        dir = "TEST_bigbed_file";
        name = "bigbed_file";
    }

    std::filesystem::path dir;
    std::string name;

    template<typename ... Args_>
    void expect_error(const std::string& msg, Args_&& ... args) {
        expect_validation_error(dir, msg, std::forward<Args_>(args)...);
    }
};

TEST_F(BigBedFileTest, Basic) {
    initialize_directory_simple(dir, name, "2.0");
    expect_error("unsupported version");

    initialize_directory_simple(dir, name, "1.0");
    {
        std::ofstream handle(dir / "file.bb");
        handle << "foobar";
    }
    expect_error("incorrect bigBed file signature");

    {
        byteme::RawFileWriter handle(dir / "file.bb");
        uint32_t val = 0x8789F2EB;
        handle.write(reinterpret_cast<unsigned char*>(&val), sizeof(val));
    }
    test_validate(dir);

    {
        byteme::RawFileWriter handle(dir / "file.bb");
        uint32_t val = 0xEBF28987;
        handle.write(reinterpret_cast<unsigned char*>(&val), sizeof(val));
    }
    test_validate(dir);
}

TEST_F(BigBedFileTest, Strict) {
    initialize_directory_simple(dir, name, "1.0");

    {
        byteme::RawFileWriter handle(dir / "file.bb");
        uint32_t val = 0xEBF28987;
        handle.write(reinterpret_cast<unsigned char*>(&val), sizeof(val));
    }

    takane::Options opts;
    opts.bigbed_file_strict_check = [](const std::filesystem::path&, const takane::ObjectMetadata&, takane::Options&) {};
    test_validate(dir);

    opts.bigbed_file_strict_check = [](const std::filesystem::path&, const takane::ObjectMetadata&, takane::Options&) { throw std::runtime_error("ARGH"); };
    expect_error("ARGH", opts);
}
