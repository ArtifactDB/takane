#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "mock_sequence_information.h"
#include "utils.h"

#include <fstream>
#include <string>

static H5::Group mock_sequence_information(const std::filesystem::path& dir) {
    return mock_sequence_information(
        dir,
        { "chrA", "chrB", "chrC" },
        { 4, 9, 19 },
        { 1, 0, 1 },
        { "mm10", "hg19", "rn10" }
    );
}

TEST(SequenceInformation, Okay) {
    auto dir = define_test_path("sequence_information");

    {
        mock_sequence_information(dir);
    }
    test_validate(dir);
}

TEST(SequenceInformation, VersionError) {
    auto dir = define_test_path("sequence_information");

    {
        initialize_directory_simple(dir, "sequence_information", "2.0");
    }
    expect_validation_error(dir, "unsupported version");
}

TEST(SequenceInformation, NameError) {
    auto dir = define_test_path("sequence_information");

    {
        auto ghandle = mock_sequence_information(dir);
        ghandle.unlink("name");
        add_hdf5_dataset(ghandle, "name", H5::PredType::NATIVE_INT, 3);
    }
    expect_validation_error(dir, "UTF-8 encoded string");

    {
        auto ghandle = mock_sequence_information(dir);
        ghandle.unlink("name");
        ghandle.createDataSet("name", H5::StrType(0, 10), H5S_SCALAR);
    }
    expect_validation_error(dir, "1-dimensional dataset");

    {
        mock_sequence_information(
            dir,
            { "chrA", "chrB", "chrA" },
            { 4, 9, 19 },
            { 1, 0, 1 },
            { "mm10", "hg19", "rn10" }
        );
    }
    expect_validation_error(dir, "duplicated sequence name");
}

/******************************************/

TEST(SequenceInformation, LengthError) {
    auto dir = define_test_path("sequence_information");

    {
        auto ghandle = mock_sequence_information(dir);
        ghandle.unlink("length");
        add_hdf5_dataset(ghandle, "length", H5::PredType::NATIVE_FLOAT, 3);
    }
    expect_validation_error(dir, "64-bit unsigned integer");

    {
        auto ghandle = mock_sequence_information(dir);
        ghandle.unlink("length");
        ghandle.createDataSet("length", H5::PredType::NATIVE_UINT8, H5S_SCALAR);
    }
    expect_validation_error(dir, "1-dimensional");

    {
        auto ghandle = mock_sequence_information(dir);
        ghandle.unlink("length");
        add_hdf5_dataset(ghandle, "length", H5::PredType::NATIVE_UINT8, 4);
    }
    expect_validation_error(dir, "same as that of 'name'");
}

TEST(SequenceInformation, LengthMissingOkay) {
    auto dir = define_test_path("sequence_information");

    {
        auto ghandle = mock_sequence_information(dir);
        auto lhandle = ghandle.openDataSet("length");
        lhandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_UINT32, H5S_SCALAR);
    }
    test_validate(dir);
}

TEST(SequenceInformation, LengthMissingError) {
    auto dir = define_test_path("sequence_information");

    {
        auto ghandle = mock_sequence_information(dir);
        auto lhandle = ghandle.openDataSet("length");
        lhandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_INT, H5S_SCALAR);
    }
    expect_validation_error(dir, "same datatype as");
}

/******************************************/

TEST(SequenceInformation, CircularError) {
    auto dir = define_test_path("sequence_information");

    {
        auto ghandle = mock_sequence_information(dir);
        ghandle.unlink("circular");
        add_hdf5_dataset(ghandle, "circular", H5::PredType::NATIVE_FLOAT, 3);
    }
    expect_validation_error(dir, "32-bit signed integer");

    {
        auto ghandle = mock_sequence_information(dir);
        ghandle.unlink("circular");
        ghandle.createDataSet("circular", H5::PredType::NATIVE_INT8, H5S_SCALAR);
    }
    expect_validation_error(dir, "1-dimensional");

    {
        auto ghandle = mock_sequence_information(dir);
        ghandle.unlink("circular");
        add_hdf5_dataset(ghandle, "circular", H5::PredType::NATIVE_INT8, 2);
    }
    expect_validation_error(dir, "same as that of 'name'");
}

TEST(SequenceInformation, CircularMissingOkay) {
    auto dir = define_test_path("sequence_information");

    {
        auto ghandle = mock_sequence_information(dir);
        auto chandle = ghandle.openDataSet("circular");
        chandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_INT8, H5S_SCALAR);
    }
    test_validate(dir);
}

TEST(SequenceInformation, CircularMissingError) {
    auto dir = define_test_path("sequence_information");

    {
        auto ghandle = mock_sequence_information(dir);
        auto chandle = ghandle.openDataSet("circular");
        chandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_INT32, H5S_SCALAR);
    }
    expect_validation_error(dir, "same datatype");
}

/******************************************/

TEST(SequenceInformation, GenomeError) {
    auto dir = define_test_path("sequence_information");

    {
        auto ghandle = mock_sequence_information(dir);
        ghandle.unlink("genome");
        add_hdf5_dataset(ghandle, "genome", H5::PredType::NATIVE_INT, 3);
    }
    expect_validation_error(dir, "UTF-8 encoded string");

    {
        auto ghandle = mock_sequence_information(dir);
        ghandle.unlink("genome");
        ghandle.createDataSet("genome", H5::StrType(0, 10), H5S_SCALAR);
    }
    expect_validation_error(dir, "1-dimensional");

    {
        auto ghandle = mock_sequence_information(dir);
        ghandle.unlink("genome");
        add_hdf5_dataset(ghandle, "genome", H5::StrType(0, 10), 5);
    }
    expect_validation_error(dir, "same as that of 'name'");
}

TEST(SequenceInformation, GenomeMissingOkay) {
    auto dir = define_test_path("sequence_information");

    {
        auto ghandle = mock_sequence_information(dir);
        auto dhandle = ghandle.openDataSet("genome");
        dhandle.createAttribute("missing-value-placeholder", H5::StrType(0, 10), H5S_SCALAR);
    }
    test_validate(dir);
}

TEST(SequenceInformation, GenomeMissingError) {
    auto dir = define_test_path("sequence_information");

    {
        auto ghandle = mock_sequence_information(dir);
        auto dhandle = ghandle.openDataSet("genome");
        dhandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_UINT16, H5S_SCALAR);
    }
    expect_validation_error(dir, "UTF-8 string");
}
