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
  Each integer is a 0-based index into the array of factor levels, found in the `data_frame.columns.levels` property.
  Each integer should be non-negative and less than the total number of levels, or equal to the missing value placeholder (see below).
- **For `data_frame.version = 1`:** A factor column is represented by any string dataset.
  Each entry in the string dataset should either be present in the set of levels or be equal to the missing placeholder value (see below).
- Floating-point columns can be represented by any floating-point datatype at the discretion of the data generator.
  IEEE special values like Inf and NaN are allowed.
- String columns can be represented by any string datatype (fixed or variable, ASCII or UTF-8) at the discretion of the data generator.
  The character encoding specified in the dataset's type should be respected.
  Non-missing strings may be associated with further format constraints based on the `data_frame.columns.format` property, which may be one of the following:
    - No format constraints.
    - String must be a `YYYY-MM-DD` date.
    - String must be an Internet date/time complying with the RFC3339 specification.

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

### CSV data frame

A data frame object stored inside a CSV file, formatted as described in the [**comservatory** specification (version 1.0)](https://github.com/ArtifactDB/comservatory).
This corresponds to the [`csv_data_frame`](https://github.com/ArtifactDB/BiocObjectSchemas/raw/master/raw/csv_data_frame/v1.json) schema,
of which several properties are worth noting:

- The `data_frame.version` property has a maximum value of 2.

The type of each column is determined from the corresponding `data_frame.columns.type` property in the schema:

- Boolean columns are stored as **comservatory** boolean columns. 
- Number columns are stored as **comservatory** number columns. 
- An integer column is stored as a **comservatory** integer column, where all (non-missing) values must be representable by a 32-bit signed integer.
- **For `data_frame.version >= 2`:** A factor column is represented as a **comservatory** integer column containing 0-based indices into the array of levels.
  All (non-missing) values are non-negative and less than the total number of levels.
- **For `data_frame.version = 1`:** A factor column is represented as a **comservatory** string dataset.
  Each (non-missing) entry in the string dataset should either be present in the set of levels or be equal to the missing placeholder value (see below).
- String columns are stored as **comservatory** string columns. 
  Strings may be associated with further format constraints based on the `data_frame.columns.format` property, which may be one of the following:
    - No format constraints.
    - String must be a `YYYY-MM-DD` date.
    - String must be an Internet date/time complying with the RFC3339 specification.

For non-atomic columns, a placeholder column should be present in the CSV.
This placeholder may be of any type as it will be ignored by readers.

If row names are present, they should be present in the first column of the CSV as strings.
All row names should be non-missing.

<details>
<summary>Example usage</summary>

Here we validate a CSV data frame with columns of different types and row names:

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

takane::data_frame::validate_csv(
    path, 
    /* num_rows = */ 9876, 
    /* has_row_names = */ true, 
    /* columns = */ expected_columns
);
```

Note that the row name column does not need to be considered in the set of `expected_columns`;
it is handled separately by the `has_row_names = true` argument.
</details>

### Genomic ranges

A `GenomicRanges` object stored inside a CSV file, formatted as described in the [**comservatory** specification (version 1.0)](https://github.com/ArtifactDB/comservatory).
This corresponds to the [`genomic_ranges`](https://github.com/ArtifactDB/BiocObjectSchemas/raw/master/raw/genomic_ranges/v1.json) schema.
We expect the columns in the following type and order:

- (optional) a column of strings containing names for each genomic range.
  This may have any column name.
  All strings should be non-missing.
- A column named `seqnames`, containing strings with the reference sequence (e.g., chromosome) name for each genomic range.
  All strings should be non-missing and belong to the set of known sequences in the corresponding `sequence_information` object.
- A column named `start`, containing the 1-based start position of each range on its reference sequence.
  All values are represented by 32-bit signed integers; negative values are allowed.
  No values should be missing.
- A column named `end`, containing the 1-based end position (inclusive) of each range.
  All values are represented by 32-bit signed integers; negative values are allowed.
  No values should be missing.
  The `end` value for each range should be greater than or equal to `start - 1`.
- A column named `strand`, containing the strand of each range.
  This should be one of the following strings: `+`, `-` or `*`.
  No values should be missing.

<details>
<summary>Example usage</summary>

Here we validate a `genomic_ranges`:

```cpp
#include "takane/takane.hpp"

std::unordered_set<std::string> allowed{ "chrA", "chrB" };

takane::genomic_ranges::validate(
    path, 
    /* num_ranges = */ 192, 
    /* has_names = */ true, 
    /* seqnames = */ allowed
);
```
</details>

### Sequence information

A `Seqinfo` object stored inside a CSV file, formatted as described in the [**comservatory** specification (version 1.0)](https://github.com/ArtifactDB/comservatory).
This corresponds to the [`sequence_information`](https://github.com/ArtifactDB/BiocObjectSchemas/raw/master/raw/sequence_information/v1.json) schema.
We expect the columns in the following type and order:

- A string column named `seqnames`, containing the reference sequence (e.g., chromosome) name.
  All strings should be non-missing and unique.
- An integer column named `seqlengths`, containing the length of each reference sequence.
  All values should be non-negative and fit inside a 32-bit signed integer.
  Missing values are allowed.
- A boolean column named `isCircular`, specifying whether the reference sequence is circular.
  Missing values are allowed.
- A string column named `genome`, containing the genome of origin for each sequence.
  Missing values are allowed.

<details>
<summary>Example usage</summary>

```cpp
#include "takane/takane.hpp"

auto output = takane::sequence_information::validate(
    path, 
    /* num_ranges = */ 192, 
    /* has_names = */ true, 
    /* seqnames = */ allowed
);
```

The value of `output.seqnames` can be used to define the set of allowed sequence names in `genomic_ranges::validate()`.
</details>

### Atomic vector

An atomic vector stored inside a CSV file, formatted as described in the [**comservatory** specification (version 1.0)](https://github.com/ArtifactDB/comservatory).
This corresponds to the [`atomic_vector`](https://github.com/ArtifactDB/BiocObjectSchemas/raw/master/raw/atomic_vector/v1.json) schema.
We expect the columns in the following type and order:

- (optional) A string column containing the name of each element.
  All strings should be non-missing.
  This column can have any name that not conflict with subsequent columns.
- A column named `values`, containing the contents of the atomic vector.
  This can be a string, integer, number or boolean column, and may contain missing values.
  For integer columns, all (non-missing) values must be representable by a 32-bit signed integer.

<details>
<summary>Example usage</summary>

```cpp
#include "takane/takane.hpp"

takane::atomic_vector::validate(
    path, 
    /* length = */ 99, 
    /* has_names = */ true
);
```
</details>

### Factor

An abstract factor stored inside a CSV file, formatted as described in the [**comservatory** specification (version 1.0)](https://github.com/ArtifactDB/comservatory).
This corresponds to the [`factor`](https://github.com/ArtifactDB/BiocObjectSchemas/raw/master/raw/factor/v1.json) schema.
We expect the columns in the following type and order:

- (optional) A string column containing the name of each element.
  All strings should be non-missing.
  This column can have any name that not conflict with subsequent columns.
- An integer column named `values`, containing the codes for the factor.
  Missing values are allowed.
  All (non-missing) values must be non-negative, less than the total number of levels, and representable by a 32-bit signed integer.

<details>
<summary>Example usage</summary>

```cpp
#include "takane/takane.hpp"

takane::factor::validate(
    path, 
    /* length = */ 99, 
    /* num_levels = */ 20, 
    /* has_names = */ false
);
```
</details>

### String factor

A factor with string levels, corresponding to the [`string_factor`](https://github.com/ArtifactDB/BiocObjectSchemas/raw/master/raw/string_factor/v1.json) schema.
The file at `path` can be validated as described for the [Factor](#Factor).
The levels at `string_factor.levels` should be stored inside a CSV file, 
formatted as described in the [**comservatory** specification (version 1.0)](https://github.com/ArtifactDB/comservatory).
We expect a single string column containing unique and non-missing levels.

<details>
<summary>Example usage</summary>

```cpp
#include "takane/takane.hpp"

takane::string_factor::validate_levels(
    levels_path, 
    /* length = */ 99
);
```
</details>

### Compressed list

An abstract compressed list stored inside a CSV file, formatted as described in the [**comservatory** specification (version 1.0)](https://github.com/ArtifactDB/comservatory).
This corresponds to the [`compressed_list`](https://github.com/ArtifactDB/BiocObjectSchemas/raw/master/raw/compressed_list/v1.json) schema.
We expect the columns in the following type and order:

- (optional) A string column containing the name of each element.
  All strings should be non-missing.
  This column can have any name that not conflict with subsequent columns.
- An integer column named `number`, containing the length of each entry of the list.
  All values must be non-negative, non-missing, and representable by a 32-bit signed integer.
  The sum of values should be equal to the total length of the concatenated object.

<details>
<summary>Example usage</summary>

```cpp
#include "takane/takane.hpp"

takane::compressed_list::validate_levels(
    levels_path, 
    /* length = */ 99,
    /* concatenated = */ 1191,
    /* has_names = */ false 
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

## Further comments

This library is named after [Takane Shijou](https://myanimelist.net/character/40012/Takane_Shijou),
continuing my trend of naming C++ libraries after [iDOLM@STER](https://myanimelist.net/anime/10278/The_iDOLMSTER) characters.

![Takane GIF](https://64.media.tumblr.com/17ecbb29ab7ed3328ed854c1b02e0eec/tumblr_o49c7i4jUu1th93f0o1_540.gif)
