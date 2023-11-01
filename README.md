# Miscellaneous validators for ArtifactDB 

## Overview

This library contains some C++ libraries to validate ArtifactDB file formats that aren't covered by other libraries
(e.g., [**comservatory**](https://github.com/ArtifactDB/comservatory), [**uzuki2**](https://github.com/ArtifactDB/uzuki2)).
The idea is to provide a cross-language method for validating the files - 
which is not quite as useful as a library for _reading_ the files, but it's better than nothing.

## Specifications

Currently, **takane** performs validation of the following representations:

- [HDF5 data frame](docs/specifications/hdf5_data_frame/v1.md)
- [CSV data frame](docs/specifications/csv_data_frame/v1.md)
- [Atomic vector](docs/specifications/atomic_vector/v1.md)
- [Factor](docs/specifications/factor/v1.md)
- [Genomic ranges](docs/specifications/genomic_ranges/v1.md)
- [HDF5 dense array](docs/specifications/hdf5_dense_array/v1.md)
- [HDF5 sparse matrix](docs/specifications/hdf5_sparse_matrix/v1.md)
- [Sequence information](docs/specifications/sequence_information/v1.md)

Each representation has its own validation function that takes information from the schema and checks them against the file contents.
For example, for the `hdf5_sparse_matrix`, we could do:

```cpp
#include "takane/takane.hpp"

takane::hdf5_sparse_matrix::Parameters params(group_name, { 10, 20 });
params.type = takane::array::Type::BOOLEAN;

takane::hdf5_sparse_matrix::validate(file_path, params);
```

Check out the [reference documentation](https://artifactdb.github.io/takane/) for more details.

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
