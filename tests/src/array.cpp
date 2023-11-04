#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "takane/array.hpp"

struct ArrayUtilsCheckDataTest : public ::testing::TestWithParam<int> {
    ArrayUtilsCheckDataTest() {
        path = "TEST-array_utils.h5";
        name = "array";
    }

    std::string path, name;

public:
    static void attach_type(H5::DataSet& handle, const std::string& type) {
        H5::StrType stype(0, H5T_VARIABLE);
        auto ahandle = handle.createAttribute("type", stype, H5S_SCALAR);
        ahandle.write(stype, type);
    }

    template<typename ... Args_>
    void expect_error(const std::string& msg, Args_&& ... args) {
        H5::H5File handle(path, H5F_ACC_RDONLY);
        auto dhandle = handle.openDataSet(name);
        EXPECT_ANY_THROW({
            try {
                takane::array::check_data(dhandle, std::forward<Args_>(args)...);
            } catch (std::exception& e) {
                EXPECT_THAT(e.what(), ::testing::HasSubstr(msg));
                throw;
            }
        });
    }
};

TEST_P(ArrayUtilsCheckDataTest, IntegerFails) {
    auto version = GetParam();
    ritsuko::Version new_version;
    if (version >= 3) {
        new_version.major = 1;
    } 

    std::vector<hsize_t> dims{ 20, 30 };
    H5::DataSpace dspace(dims.size(), dims.data());

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto xhandle = handle.createDataSet(name, H5::PredType::NATIVE_INT, dspace);
        if (version >= 3) {
            attach_type(xhandle, "integer");
        }
    }
    {
        H5::H5File handle(path, H5F_ACC_RDONLY);
        auto dhandle = handle.openDataSet(name);
        takane::array::check_data(dhandle, takane::array::Type::INTEGER, new_version, version);
    }

    // Wrong type attribute.
    if (version >= 3) {
        {
            H5::H5File handle(path, H5F_ACC_RDWR);
            auto xhandle = handle.openDataSet(name);
            xhandle.removeAttr("type");
            attach_type(xhandle, "number");
        }
        expect_error("expected 'type' attribute", takane::array::Type::INTEGER, new_version, version);
    }

    // Wrong data type.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto xhandle = handle.createDataSet(name, H5::StrType(0, H5T_VARIABLE), dspace);
        if (version >= 3) {
            attach_type(xhandle, "integer");
        }
    }
    if (version < 3) {
        expect_error("expected an integer type", takane::array::Type::INTEGER, new_version, version);
    } else {
        expect_error("32-bit signed integer", takane::array::Type::INTEGER, new_version, version);
    }
}

TEST_P(ArrayUtilsCheckDataTest, BooleanFails) {
    auto version = GetParam();
    ritsuko::Version new_version;
    if (version >= 3) {
        new_version.major = 1;
    } 

    std::vector<hsize_t> dims{ 20, 30 };
    H5::DataSpace dspace(dims.size(), dims.data());

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto xhandle = handle.createDataSet(name, H5::PredType::NATIVE_INT, dspace);
        if (version >= 3) {
            attach_type(xhandle, "boolean");
        }
    }
    {
        H5::H5File handle(path, H5F_ACC_RDONLY);
        auto dhandle = handle.openDataSet(name);
        takane::array::check_data(dhandle, takane::array::Type::BOOLEAN, new_version, version);
    }

    // Wrong type attribute.
    if (version >= 3) {
        {
            H5::H5File handle(path, H5F_ACC_RDWR);
            auto xhandle = handle.openDataSet(name);
            xhandle.removeAttr("type");
            attach_type(xhandle, "number");
        }
        expect_error("expected 'type' attribute", takane::array::Type::BOOLEAN, new_version, version);
    }

    // Wrong data type.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto xhandle = handle.createDataSet(name, H5::StrType(0, H5T_VARIABLE), dspace);
        if (version >= 3) {
            attach_type(xhandle, "boolean");
        }
    }
    if (version < 3) {
        expect_error("expected an integer type", takane::array::Type::BOOLEAN, new_version, version);
    } else {
        expect_error("32-bit signed integer", takane::array::Type::BOOLEAN, new_version, version);
    }
}

TEST_P(ArrayUtilsCheckDataTest, NumberFails) {
    auto version = GetParam();
    ritsuko::Version new_version;
    if (version >= 3) {
        new_version.major = 1;
    } 

    std::vector<hsize_t> dims{ 20, 30 };
    H5::DataSpace dspace(dims.size(), dims.data());

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto xhandle = handle.createDataSet(name, H5::PredType::NATIVE_INT, dspace);
        if (version >= 3) {
            attach_type(xhandle, "number");
        }
    }
    {
        H5::H5File handle(path, H5F_ACC_RDONLY);
        auto dhandle = handle.openDataSet(name);
        takane::array::check_data(dhandle, takane::array::Type::NUMBER, new_version, version);
    }

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        std::vector<hsize_t> dims{ 20, 30 };
        H5::DataSpace dspace(dims.size(), dims.data());
        auto xhandle = handle.createDataSet(name, H5::PredType::NATIVE_FLOAT, dspace);
        if (version >= 3) {
            attach_type(xhandle, "number");
        }
    }
    {
        H5::H5File handle(path, H5F_ACC_RDONLY);
        auto dhandle = handle.openDataSet(name);
        takane::array::check_data(dhandle, takane::array::Type::NUMBER, new_version, version);
    }

    // Wrong type attribute.
    if (version >= 3) {
        {
            H5::H5File handle(path, H5F_ACC_RDWR);
            auto xhandle = handle.openDataSet(name);
            xhandle.removeAttr("type");
            attach_type(xhandle, "boolean");
        }
        expect_error("expected 'type' attribute", takane::array::Type::NUMBER, new_version, version);
    }

    // Wrong data type.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto xhandle = handle.createDataSet(name, H5::StrType(0, H5T_VARIABLE), dspace);
        if (version >= 3) {
            attach_type(xhandle, "number");
        }
    }
    if (version < 3) {
        expect_error("expected an integer or floating-point type", takane::array::Type::NUMBER, new_version, version);
    } else {
        expect_error("64-bit float", takane::array::Type::NUMBER, new_version, version);
    }
}

TEST_P(ArrayUtilsCheckDataTest, StringFails) {
    auto version = GetParam();
    ritsuko::Version new_version;
    if (version >= 3) {
        new_version.major = 1;
    } 

    std::vector<hsize_t> dims{ 20, 30 };
    H5::DataSpace dspace(dims.size(), dims.data());
    H5::StrType stype(0, 10);

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto xhandle = handle.createDataSet(name, stype, dspace);
        if (version >= 3) {
            attach_type(xhandle, "string");
        }
    }
    {
        H5::H5File handle(path, H5F_ACC_RDONLY);
        auto dhandle = handle.openDataSet(name);
        takane::array::check_data(dhandle, takane::array::Type::STRING, new_version, version);
    }

    // Wrong type attribute.
    if (version >= 3) {
        {
            H5::H5File handle(path, H5F_ACC_RDWR);
            auto xhandle = handle.openDataSet(name);
            xhandle.removeAttr("type");
            attach_type(xhandle, "boolean");
        }
        expect_error("expected 'type' attribute", takane::array::Type::STRING, new_version, version);
    }

    // Wrong data type.
    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto xhandle = handle.createDataSet(name, H5::PredType::NATIVE_INT, dspace);
        if (version >= 3) {
            attach_type(xhandle, "string");
        }
    }
    expect_error("expected a string type", takane::array::Type::STRING, new_version, version);
}

TEST_P(ArrayUtilsCheckDataTest, MissingPlaceholder) {
    auto version = GetParam();
    ritsuko::Version new_version;
    if (version >= 3) {
        new_version.major = 1;
    } 

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        std::vector<hsize_t> dims{ 20, 30 };
        H5::DataSpace dspace(dims.size(), dims.data());
        auto xhandle = handle.createDataSet(name, H5::PredType::NATIVE_INT8, dspace);
        if (version >= 3) {
            attach_type(xhandle, "number");
        }
        xhandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_INT8, H5S_SCALAR);
    }
    {
        H5::H5File handle(path, H5F_ACC_RDONLY);
        auto dhandle = handle.openDataSet(name);
        takane::array::check_data(dhandle, takane::array::Type::NUMBER, new_version, version);
    }

    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        auto xhandle = handle.openDataSet(name);
        xhandle.removeAttr("missing-value-placeholder");
        xhandle.createAttribute("missing-value-placeholder", H5::PredType::NATIVE_INT32, H5S_SCALAR);
    }
    if (version == 1) {
        H5::H5File handle(path, H5F_ACC_RDONLY);
        auto dhandle = handle.openDataSet(name);
        takane::array::check_data(dhandle, takane::array::Type::NUMBER, new_version, version);
    } else {
        expect_error("missing-value-placeholder", takane::array::Type::NUMBER, new_version, version);
    }
}

INSTANTIATE_TEST_SUITE_P(
    ArrayUtilsCheckData,
    ArrayUtilsCheckDataTest,
    ::testing::Values(1,2,3)
);

struct ArrayUtilsCheckDimnamesTest : public ::testing::Test {
    ArrayUtilsCheckDimnamesTest() {
        path = "TEST-array_utils.h5";
        name = "array";
    }

    std::string path, name;

public:
    template<class Object_>
    static void attach_dimnames(Object_& handle, const std::vector<const char*>& buffer) {
        H5::StrType stype(0, H5T_VARIABLE);
        hsize_t dim = buffer.size();
        H5::DataSpace dspace(1, &dim);
        auto attr = handle.createAttribute("dimension-names", stype, dspace);
        attr.write(stype, buffer.data());
    }

    template<typename ... Args_>
    static void expect_error(const std::string& msg, Args_&& ... args) {
        EXPECT_ANY_THROW({
            try {
                takane::array::check_dimnames(std::forward<Args_>(args)...);
            } catch (std::exception& e) {
                EXPECT_THAT(e.what(), ::testing::HasSubstr(msg));
                throw;
            }
        });
    }

    template<typename ... Args_>
    static void expect_error2(const std::string& msg, Args_&& ... args) {
        EXPECT_ANY_THROW({
            try {
                takane::array::check_dimnames2(std::forward<Args_>(args)...);
            } catch (std::exception& e) {
                EXPECT_THAT(e.what(), ::testing::HasSubstr(msg));
                throw;
            }
        });
    }
};

TEST_F(ArrayUtilsCheckDimnamesTest, Old) {
    std::string dname = "dimnames";
    std::vector<size_t> dims { 10, 20 };

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        handle.createGroup(dname);
    }
    {
        H5::H5File handle(path, H5F_ACC_RDONLY);
        takane::array::check_dimnames(handle, dname, dims);
    }

    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        auto nhandle = handle.openGroup(dname);
        hsize_t dim = dims[1];
        H5::DataSpace dspace(1, &dim);
        nhandle.createDataSet("1", H5::StrType(0, H5T_VARIABLE), dspace);
    }
    {
        H5::H5File handle(path, H5F_ACC_RDONLY);
        takane::array::check_dimnames(handle, dname, dims);
    }

    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        auto nhandle = handle.openGroup(dname);
        hsize_t dim = dims[0];
        H5::DataSpace dspace(1, &dim);
        nhandle.createDataSet("0", H5::StrType(0, H5T_VARIABLE), dspace);
    }
    {
        H5::H5File handle(path, H5F_ACC_RDONLY);
        takane::array::check_dimnames(handle, dname, dims);
    }

    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        auto ghandle = handle.openGroup(dname);
        ghandle.unlink("0");
        ghandle.createGroup("0");
    }
    {
        H5::H5File handle(path, H5F_ACC_RDONLY);
        expect_error("expected '0' to be a dataset", handle, dname, dims);
    }

    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        auto ghandle = handle.openGroup(dname);
        ghandle.unlink("0");
        ghandle.createDataSet("0", H5::PredType::NATIVE_INT, H5S_SCALAR);
    }
    {
        H5::H5File handle(path, H5F_ACC_RDONLY);
        expect_error("expected a string dataset", handle, dname, dims);
    }

    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        auto ghandle = handle.openGroup(dname);
        ghandle.unlink("0");
        hsize_t dim = dims[0] + 5;
        H5::DataSpace dspace(1, &dim);
        ghandle.createDataSet("0", H5::StrType(0, H5T_VARIABLE), dspace);
    }
    {
        H5::H5File handle(path, H5F_ACC_RDONLY);
        expect_error("expected dataset to have length", handle, dname, dims);
    }
}

TEST_F(ArrayUtilsCheckDimnamesTest, New) {
    std::vector<size_t> dims { 10, 20 };
    H5::StrType stype(0, H5T_VARIABLE);
    hsize_t ndims = dims.size();
    H5::DataSpace attspace(1, &ndims);
    const char* empty = "";
    std::vector<const char*> buffer { empty, empty };

    {
        H5::H5File handle(path, H5F_ACC_TRUNC);
        auto ghandle = handle.createGroup(name);
        attach_dimnames(ghandle, buffer);
    }
    {
        H5::H5File handle(path, H5F_ACC_RDONLY);
        auto ghandle = handle.openGroup(name);
        takane::array::check_dimnames2(handle, ghandle, dims);
    }

    const char* colnames = "foo";
    buffer[1] = colnames;
    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        auto ghandle = handle.openGroup(name);
        auto ahandle = ghandle.openAttribute("dimension-names");
        ahandle.write(stype, buffer.data());
    }
    {
        H5::H5File handle(path, H5F_ACC_RDONLY);
        auto ghandle = handle.openGroup(name);
        expect_error2("expected a dataset", handle, ghandle, dims);
    }

    hsize_t nrow = dims[0];
    H5::DataSpace rowspace(1, &nrow);
    hsize_t ncol = dims[1];
    H5::DataSpace colspace(1, &ncol);
    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        handle.createDataSet(colnames, H5::PredType::NATIVE_INT, colspace);
    }
    {
        H5::H5File handle(path, H5F_ACC_RDONLY);
        auto ghandle = handle.openGroup(name);
        expect_error2("expected a string dataset", handle, ghandle, dims);
    }

    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        handle.unlink(colnames);
        handle.createDataSet(colnames, stype, rowspace);
    }
    {
        H5::H5File handle(path, H5F_ACC_RDONLY);
        auto ghandle = handle.openGroup(name);
        expect_error2("expected dataset to have length", handle, ghandle, dims);
    }

    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        handle.unlink(colnames);
        handle.createDataSet(colnames, stype, colspace);
    }
    {
        H5::H5File handle(path, H5F_ACC_RDONLY);
        auto ghandle = handle.openGroup(name);
        takane::array::check_dimnames2(handle, ghandle, dims);
    }

    const char* rownames = "whee";
    buffer[0] = rownames;
    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        auto ghandle = handle.openGroup(name);
        auto ahandle = ghandle.openAttribute("dimension-names");
        ahandle.write(stype, buffer.data());
        handle.createDataSet(rownames, H5::StrType(0, H5T_VARIABLE), rowspace);
    }
    {
        H5::H5File handle(path, H5F_ACC_RDONLY);
        auto ghandle = handle.openGroup(name);
        takane::array::check_dimnames2(handle, ghandle, dims);
    }

    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        auto ghandle = handle.openGroup(name);
        ghandle.removeAttr("dimension-names");
    }
    {
        H5::H5File handle(path, H5F_ACC_RDONLY);
        auto ghandle = handle.openGroup(name);
        expect_error2("expected a 'dimension-names' attribute", handle, ghandle, dims);
    }

    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        auto ghandle = handle.openGroup(name);
        ghandle.createAttribute("dimension-names", H5::PredType::NATIVE_INT, H5S_SCALAR);
    }
    {
        H5::H5File handle(path, H5F_ACC_RDONLY);
        auto ghandle = handle.openGroup(name);
        expect_error2("'dimension-names' attribute should have a string", handle, ghandle, dims);
    }

    {
        H5::H5File handle(path, H5F_ACC_RDWR);
        auto ghandle = handle.openGroup(name);
        hsize_t ndims = 10;
        H5::DataSpace attspace(1, &ndims);
        ghandle.removeAttr("dimension-names");
        ghandle.createAttribute("dimension-names", stype, attspace);
    }
    {
        H5::H5File handle(path, H5F_ACC_RDONLY);
        auto ghandle = handle.openGroup(name);
        expect_error2("'dimension-names' attribute should have length equal to the number of dimensions (2)", handle, ghandle, dims);
    }
}
