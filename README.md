# Miscellaneous validators for ArtifactDB 

## Overview

This library contains some C++ libraries to validate ArtifactDB file formats that aren't covered by other libraries
(e.g., [**comservatory**](https://github.com/ArtifactDB/comservatory), [**uzuki2**](https://github.com/ArtifactDB/uzuki2)).
The idea is to provide a cross-language method for validating the files - 
which is not quite as useful as a library for _reading_ the files, but it's better than nothing.

## Specifications

### HDF5 data frame

A data frame object stored inside a group of a HDF5 file.
This corresponds to the [`hdf5_data_frame`](https://github.com/ArtifactDB/BiocObjectSchemas/raw/master/raw/hdf5_data_frame/v1.json) schema,
of which several properties are worth noting:

- The name of the group is specified in the `hdf5_data_frame.group` property. 
- The `data_frame.version` property has a maximum value of 2.
- The `hdf5_data_frame.version` property has a maximum value of 2.

Each atomic column is stored as a 1-dimensional dataset in the `data` subgroup, named by its positional 0-based index in the data frame,
e.g., the first column is named `0`, the second column is named `1`, and so on.
All datasets representing atomic columns should have the same length.
The type of each atomic column is determined from the corresponding `data_frame.columns.type` property in the schema:

- A boolean column is stored as an integer HDF5 dataset where a value of 1 is truthy and a value of zero is falsey.
  Any integer data type can be used at the discretion of the data generator, though the type's range of values must be representable by a 32-bit signed integer.
- An integer column can be represented by any integer HDF5 dataset.
  Any integer data type can be used at the discretion of the data generator, though the type's range of values must be representable by a 32-bit signed integer.
- **For `data_frame.version >= 2`:** A factor column is represented by a integer HDF5 dataset.
  Any integer data type can be used at the discretion of the data generator, though the type's range of values must be representable by a 32-bit signed integer.
  Each integer is an index into the array of factor levels, found in the `data_frame.columns.levels` property.
  Each integer should be non-negative and less than the total number of levels, or equal to the missing value placeholder (see below).
- **For `data_frame.version = 1`:** A factor column is represented by any string dataset.
  Each entry in the string dataset should either be present in the set of levels or be equal to the missing placeholder value (see below).
- Floating-point columns can be represented by any floating-point datatype at the discretion of the data generator.
  IEEE special values like Inf and NaN are allowed.
- String columns can be represented by any string datatype (fixed or variable, ASCII or UTF-8) at the discretion of the data generator.
  The character encoding specified in the dataset's type should be respected.
  Strings may be associated with further format constraints based on the `data_frame.columns.format` property, which may be one of the following:
    - No format constraints.
    - String must be a `YYYY-MM-DD` date.
    - String must be an Internet date/time complying with the RFC3339 specification.
  Note that strings equal to the missing value placeholder are not subject to these constraints.

**For `hdf5_data_frame.version >= 2`:** Each dataset may have a `missing-value-placeholder` attribute, containing a scalar value of the same exact type.
Any value in the dataset equal to this placeholder should be treated as missing.
If no attribute exists, it can be assumed that no values are missing.
For all types except strings, the type of the scalar should b exactly the same as that of the dataset, so as to avoid transformations during casting.
For strings, the scalar value may be of any string type class, and all comparisons should be performed byte-wise like `strcmp`.
For numbers, the scalar value may be NaN with a non-default payload, which should be considered via byte-wise comparisons, e.g., with `memcmp`.

**For `hdf5_data_frame.version = 1`:** Missing integers and booleans are represented by -2147483648. 
Missing floats are always represented by NaNs with R's missingness payload.
Missing strings are represented by a `missing-value-placeholder` attribute, containing a scalar value of some string type.

For non-atomic columns, the corresponding dataset is omitted and the actual contents are obtained from other files.
A pointer to the resource should be stored in the corresponding entry of the `data_frame.columns` property.

Column names are stored in `column_names`, a 1-dimensional string dataset of length equal to the number of columns.
Row names, if present, are stored in a 1-dimensional `row_names` string dataset of the same length at the number of rows.
Neither should contain any missing values.

<details>
<summary>Example usage</summary>

Here we validate a HDF5 data frame with columns of different types and row names:

```cpp
#include "takane/takane.hpp"

std::vector<takane::data_frame::ColumnDetails> expected_columns(5);
expected_columns[0].type = takane::data_frame::ColumnType::INTEGER;
expected_columns[1].type = takane::data_frame::ColumnType::STRING;
expected_columns[2].type = takane::data_frame::ColumnType::STRING;
expected_columns[2].format = takane::data_frame::StringFormat::DATETIME;
expected_columns[3].type = takane::data_frame::ColumnType::FACTOR;
expected_columns[3].add_factor_level("foo"); // taken from 'data_frame.columns[3].levels'
expected_columns[3].add_factor_level("bar");
expected_columns[4].type = takane::data_frame::ColumnType::NUMBER;

takane::data_frame::validate_hdf5(
    path, 
    name, 
    /* num_rows = */ 9876, 
    /* has_row_names = */ true, 
    /* columns = */ expected_columns
);
```
</details>

## Building projects

### CMake with `FetchContent`

If you're using CMake, you just need to add something like this to your `CMakeLists.txt`:

```cmake
include(FetchContent)

FetchContent_Declare(
  takane 
  GIT_REPOSITORY https://github.com/ArtifactDB/takane
  GIT_TAG master # or any version of interest
)

FetchContent_MakeAvailable(takane)
```

Then you can link to **takane** to make the headers available during compilation:

```cmake
# For executables:
target_link_libraries(myexe takane)

# For libaries
target_link_libraries(mylib INTERFACE takane)
```

### CMake with `find_package()`

You can install the library by cloning a suitable version of this repository and running the following commands:

```sh
mkdir build && cd build
cmake .. -DTAKANE_TESTS=OFF
cmake --build . --target install
```

Then you can use `find_package()` as usual:

```cmake
find_package(artifactdb_takane CONFIG REQUIRED)
target_link_libraries(mylib INTERFACE artifactdb::takane)
```

### Manual

If you're not using CMake, the simple approach is to just copy the files in the `include/` subdirectory - 
either directly or with Git submodules - and include their path during compilation with, e.g., GCC's `-I`.
You will also need to link to the dependencies listed in the [`extern/CMakeLists.txt`](extern/CMakeLists.txt) directory along with the HDF5 library.
