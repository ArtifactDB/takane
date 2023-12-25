#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "takane/takane.hpp"

#include "multi_sample_dataset.h"
#include "summarized_experiment.h"
#include "utils.h"

#include <string>
#include <vector>
#include <filesystem>
#include <stdexcept>

struct MultiSampleDatasetTest : public ::testing::Test {
    MultiSampleDatasetTest() {
        dir = "TEST_multi_sample_dataset";
        name = "multi_sample_dataset";
    }

    std::filesystem::path dir;
    std::string name;

    void expect_error(const std::string& msg) {
        EXPECT_ANY_THROW({
            try {
                takane::validate(dir);
            } catch (std::exception& e) {
                EXPECT_THAT(e.what(), ::testing::HasSubstr(msg));
                throw;
            }
        });
    }
};

TEST_F(MultiSampleDatasetTest, Metadata) {
    initialize_directory(dir);
    auto dump = [&](const std::string& contents) {
        std::ofstream handle(dir / "OBJECT");
        handle << "{ \"type\": \"" + name + "\", \"" + name + "\": " + contents + " }";
    };

    dump("{ \"version\": 1.0 }");
    expect_error("should be a JSON string");

    dump("{ \"version\": \"2.0\" }");
    expect_error("unsupported version");
}

TEST_F(MultiSampleDatasetTest, Basic) {
    // No experiments.
    multi_sample_dataset::Options opt(3);
    multi_sample_dataset::mock(dir, opt);
    takane::validate(dir); // success!

    // One experiment.
    opt.experiments.emplace_back(18, 7);
    multi_sample_dataset::mock(dir, opt);
    takane::validate(dir); // success!

    // Two experiments.
    opt.experiments.emplace_back(21, 11);
    multi_sample_dataset::mock(dir, opt);
    takane::validate(dir); // success!
}

TEST_F(MultiSampleDatasetTest, SampleMap) {
    multi_sample_dataset::Options opt(3);
    opt.experiments.emplace_back(18, 7);
    multi_sample_dataset::mock(dir, opt);

    {
        H5::H5File handle(dir / "sample_map.h5", H5F_ACC_RDWR);
        auto ghandle = handle.openGroup("multi_sample_dataset");
        ghandle.createGroup("1");
    }
    expect_error("more objects present");

    {
        H5::H5File handle(dir / "sample_map.h5", H5F_ACC_RDWR);
        auto ghandle = handle.openGroup("multi_sample_dataset");
        auto dhandle = ghandle.openDataSet("0");

        size_t len = ritsuko::hdf5::get_1d_length(dhandle, false);
        std::vector<int> dump(len);
        dhandle.read(dump.data(), H5::PredType::NATIVE_INT);
        dump[0] = 10;
        dhandle.write(dump.data(), H5::PredType::NATIVE_INT);
    }
    expect_error("less than the number of samples");

    {
        H5::H5File handle(dir / "sample_map.h5", H5F_ACC_RDWR);
        auto ghandle = handle.openGroup("multi_sample_dataset");
        ghandle.unlink("0");
        hdf5_utils::spawn_data(ghandle, "0", 7, H5::PredType::NATIVE_INT32);
    }
    expect_error("64-bit unsigned integer");

    {
        H5::H5File handle(dir / "sample_map.h5", H5F_ACC_RDWR);
        auto ghandle = handle.openGroup("multi_sample_dataset");
        ghandle.unlink("0");
        hdf5_utils::spawn_data(ghandle, "0", 10, H5::PredType::NATIVE_UINT32);
    }
    expect_error("should equal the number of columns");
}

TEST_F(MultiSampleDatasetTest, SampleData) {
    multi_sample_dataset::Options opt(4);
    opt.experiments.emplace_back(18, 7);
    multi_sample_dataset::mock(dir, opt);

    simple_list::mock(dir / "sample_data");
    expect_error("'DATA_FRAME' interface");

    initialize_directory_simple(dir / "sample_data", "data_frame", "2.0");
    expect_error("failed to validate 'sample_data'");
}

TEST_F(MultiSampleDatasetTest, Experiments) {
    multi_sample_dataset::Options opt(5);
    opt.experiments.emplace_back(18, 7);
    opt.experiments.emplace_back(21, 11);

    // Basic errors.
    {
        multi_sample_dataset::mock(dir, opt);
        simple_list::mock(dir / "experiments" / "2");
        expect_error("more objects than expected");

        simple_list::mock(dir / "experiments" / "1");
        expect_error("'SUMMARIZED_EXPERIMENT' interface");

        initialize_directory_simple(dir / "experiments" / "1", "summarized_experiment", "2.0");
        expect_error("failed to validate 'experiments/1'");
    }

    // Mismatch in columns and index dataset length for later experiments.
    {
        multi_sample_dataset::mock(dir, opt);
        H5::H5File handle(dir / "sample_map.h5", H5F_ACC_RDWR);
        auto ghandle = handle.openGroup("multi_sample_dataset");
        ghandle.unlink("1");
        hdf5_utils::spawn_data(ghandle, "1", 7, H5::PredType::NATIVE_UINT32);
    }
    expect_error("should equal the number of columns");
}

TEST_F(MultiSampleDatasetTest, OtherData) {
    multi_sample_dataset::Options opt(4);
    multi_sample_dataset::mock(dir, opt);

    dense_array::mock(dir / "other_data", dense_array::Type::INTEGER, { 10, 20 });
    expect_error("'SIMPLE_LIST' interface");

    simple_list::mock(dir / "other_data");
    takane::validate(dir);
}
