#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "sequence_information.h"
#include "utils.h"

#include <fstream>
#include <string>

struct SequenceInformationTest : public ::testing::Test {
    SequenceInformationTest() {
        dir = "TEST_seqinfo";
        name = "sequence_information";
    }

    std::filesystem::path dir;
    std::string name;

    H5::H5File initialize() {
        initialize_directory_simple(dir, "sequence_information", "1.0");
        auto path = dir / "info.h5";
        return H5::H5File(std::string(path), H5F_ACC_TRUNC);
    }

    H5::H5File reopen() {
        auto path = dir / "info.h5";
        return H5::H5File(path, H5F_ACC_RDWR);
    }

    template<typename ... Args_>
    void expect_error(const std::string& msg, Args_&& ... args) {
        expect_validation_error(dir, msg, std::forward<Args_>(args)...);
    }
};

TEST_F(SequenceInformationTest, Basic) {
    initialize_directory_simple(dir, name, "2.0");
    expect_error("unsupported version");

    {
        auto handle = initialize();
    }
    expect_error("'sequence_information'");

    {
        auto handle = reopen();
        handle.createDataSet(name, H5::PredType::NATIVE_INT, H5S_SCALAR);
    }
    expect_error("'sequence_information'");

    {
        auto handle = reopen();
        handle.unlink(name);
        auto ghandle = handle.createGroup(name);
        sequence_information::mock(
            ghandle, 
            { "chrA", "chrB", "chrC" },
            { 4, 9, 19 },
            { 1, 0, 1 },
            { "mm10", "hg19", "rn10" }
        );
    }
    test_validate(dir);
}

TEST_F(SequenceInformationTest, Seqnames) {
    {
        auto handle = initialize();
        auto ghandle = handle.createGroup(name);
        hdf5_utils::spawn_data(ghandle, "name", 10, H5::PredType::NATIVE_INT32);
    }
    expect_error("represented by a UTF-8 encoded string");

    {
        auto handle = initialize();
        auto ghandle = handle.createGroup(name);
        sequence_information::mock(
            ghandle, 
            { "chrA", "chrB", "chrA" },
            { 4, 9, 19 },
            { 1, 0, 1 },
            { "mm10", "hg19", "rn10" }
        );
    }
    expect_error("duplicated sequence name 'chrA'");
}

TEST_F(SequenceInformationTest, Seqlengths) {
    {
        auto handle = initialize();
        auto ghandle = handle.createGroup(name);
        sequence_information::mock(
            ghandle, 
            { "chrA", "chrB", "chrC" },
            { 4, 9 },
            { 1, 0, 1 },
            { "mm10", "hg19", "rn10" }
        );
    }
    expect_error("to be equal");

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        ghandle.unlink("length");
        hdf5_utils::spawn_data(ghandle, "length", 3, H5::PredType::NATIVE_INT32);
    }
    expect_error("64-bit unsigned integer");

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        ghandle.unlink("length");
        auto dhandle = hdf5_utils::spawn_data(ghandle, "length", 3, H5::PredType::NATIVE_UINT8);
        dhandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_UINT16, H5S_SCALAR);
    }
    expect_error("same type");

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        auto dhandle = ghandle.openDataSet("length");
        dhandle.removeAttr("missing-value-placeholder");
        dhandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_UINT8, H5S_SCALAR);
    }
    test_validate(dir);
}

TEST_F(SequenceInformationTest, Circular) {
    {
        auto handle = initialize();
        auto ghandle = handle.createGroup(name);
        sequence_information::mock(
            ghandle, 
            { "chrA", "chrB", "chrC" },
            { 100, 200, 300 },
            { 1, 0 },
            { "mm10", "hg19", "rn10" }
        );
    }
    expect_error("to be equal");

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        ghandle.unlink("circular");
        hdf5_utils::spawn_data(ghandle, "circular", 3, H5::PredType::NATIVE_INT64);
    }
    expect_error("32-bit signed integer");

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        ghandle.unlink("circular");
        auto dhandle = hdf5_utils::spawn_data(ghandle, "circular", 3, H5::PredType::NATIVE_UINT8);
        dhandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_UINT16, H5S_SCALAR);
    }
    expect_error("same type");

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        auto dhandle = ghandle.openDataSet("circular");
        dhandle.removeAttr("missing-value-placeholder");
        dhandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_UINT8, H5S_SCALAR);
    }
    test_validate(dir);
}

TEST_F(SequenceInformationTest, Genome) {
    {
        auto handle = initialize();
        auto ghandle = handle.createGroup(name);
        sequence_information::mock(
            ghandle, 
            { "chrA", "chrB", "chrC" },
            { 100, 200, 300 },
            { -1, 0, 1 },
            { "mm10", "hg19" }
        );
    }
    expect_error("to be equal");

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        ghandle.unlink("genome");
        hdf5_utils::spawn_data(ghandle, "genome", 3, H5::PredType::NATIVE_INT64);
    }
    expect_error("represented by a UTF-8 encoded string");

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        ghandle.unlink("genome");
        auto dhandle = hdf5_utils::spawn_string_data(ghandle, "genome", 10, { "foo", "bar", "" });
        dhandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_UINT16, H5S_SCALAR);
    }
    expect_error("same type");

    {
        auto handle = reopen();
        auto ghandle = handle.openGroup(name);
        auto dhandle = ghandle.openDataSet("genome");
        dhandle.removeAttr("missing-value-placeholder");
        dhandle.createAttribute("missing-value-placeholder", H5::StrType(0, 10), H5S_SCALAR);
    }
    test_validate(dir);
}
