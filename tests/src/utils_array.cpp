#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "takane/utils_array.hpp"

#include "utils.h"

struct ArrayUtilsTest : public::testing::Test {
    template<typename ... Args_>
    void expect_error_names(const std::string& msg, Args_&& ... args) {
        EXPECT_ANY_THROW({
            try {
                takane::internal_array::check_dimnames(std::forward<Args_>(args)...);
            } catch (std::exception& e) {
                EXPECT_THAT(e.what(), ::testing::HasSubstr(msg));
                throw;
            }
        });
    }
};

TEST_F(ArrayUtilsTest, Names) {
    std::filesystem::path path = "TEST_array_names.h5";
    std::vector<hsize_t> dims{ 10, 20 };
    // Filled is okay.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup("names");
        hdf5_utils::spawn_data(ghandle, "0", 10, H5::StrType(0, 2));
        hdf5_utils::spawn_data(ghandle, "1", 20, H5::StrType(0, 2));
    }
    {
        H5::H5File handle(path, H5F_ACC_RDONLY);
        takane::internal_array::check_dimnames(handle, "names", dims, takane::Options());
        expect_error_names("same length as the extent", handle, "names", std::vector<hsize_t>{ 20, 10 }, takane::Options());
    }

    // Empty is okay.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup("names");
    }
    {
        H5::H5File handle(path, H5F_ACC_RDONLY);
        takane::internal_array::check_dimnames(handle, "names", dims, takane::Options());
    }

    // Various failures.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup("names");
        hdf5_utils::spawn_data(ghandle, "0", 10, H5::PredType::NATIVE_INT32);
    }
    {
        H5::H5File handle(path, H5F_ACC_RDONLY);
        expect_error_names("string datatype class", handle, "names", dims, takane::Options());
    }

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup("names");
        ghandle.createGroup("1");
    }
    {
        H5::H5File handle(path, H5F_ACC_RDONLY);
        expect_error_names("to be a dataset", handle, "names", dims, takane::Options());
    }

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        hdf5_utils::spawn_data(handle, "names", 10, H5::PredType::NATIVE_INT32);
    }
    {
        H5::H5File handle(path, H5F_ACC_RDONLY);
        expect_error_names("to be a group", handle, "names", dims, takane::Options());
    }

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup("names");
        hdf5_utils::spawn_data(ghandle, "asdasd", 10, H5::PredType::NATIVE_INT32);
    }
    {
        H5::H5File handle(path, H5F_ACC_RDONLY);
        expect_error_names("more objects", handle, "names", dims, takane::Options());
    }

}

