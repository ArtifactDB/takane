# File validators for Bioconductor objects 

![Unit tests](https://github.com/ArtifactDB/takane/actions/workflows/run-tests.yaml/badge.svg)
![Documentation](https://github.com/ArtifactDB/takane/actions/workflows/doxygenate.yaml/badge.svg)
[![codecov](https://codecov.io/gh/ArtifactDB/takane/branch/master/graph/badge.svg?token=J3dxS3MtT1)](https://codecov.io/gh/ArtifactDB/takane)

## Overview

This library contains some C++ libraries to validate on-disk representations of Bioconductor objects used in ArtifactDB instances.
The idea is to provide a cross-language method for validating the files - 
which is not quite as useful as a library for _reading_ the files, but it's better than nothing.

## Specifications

See [general comments](https://github.com/ArtifactDB/takane/tree/gh-pages/docs/specifications/_general.md) for all objects' on-disk representations.

Currently, **takane** provides validators for the following objects:

- `atomic_vector_list`:
  [1.0](https://github.com/ArtifactDB/takane/tree/gh-pages/docs/specifications/atomic_vector_list/1.0.md).
- `atomic_vector`:
  [1.0](https://github.com/ArtifactDB/takane/tree/gh-pages/docs/specifications/atomic_vector/1.0.md).
- `bam_file`:
  [1.0](https://github.com/ArtifactDB/takane/tree/gh-pages/docs/specifications/bam_file/1.0.md).
- `bcf_file`:
  [1.0](https://github.com/ArtifactDB/takane/tree/gh-pages/docs/specifications/bcf_file/1.0.md).
- `bcf_file`:
  [1.0](https://github.com/ArtifactDB/takane/tree/gh-pages/docs/specifications/bcf_file/1.0.md).
- `bigbed_file`:
  [1.0](https://github.com/ArtifactDB/takane/tree/gh-pages/docs/specifications/bigbed_file/1.0.md).
- `bigwig_file`:
  [1.0](https://github.com/ArtifactDB/takane/tree/gh-pages/docs/specifications/bigwig_file/1.0.md).
- `bumpy_atomic_array`:
  [1.0](https://github.com/ArtifactDB/takane/tree/gh-pages/docs/specifications/bumpy_atomic_array/1.0.md).
- `bumpy_data_frame_array`:
  [1.0](https://github.com/ArtifactDB/takane/tree/gh-pages/docs/specifications/bumpy_data_frame_array/1.0.md).
- `compressed_sparse_matrix`:
  [1.0](https://github.com/ArtifactDB/takane/tree/gh-pages/docs/specifications/compressed_sparse_matrix/1.0.md).
- `data_frame_factor`:
  [1.0](https://github.com/ArtifactDB/takane/tree/gh-pages/docs/specifications/data_frame_factor/1.0.md).
- `data_frame_list`:
  [1.0](https://github.com/ArtifactDB/takane/tree/gh-pages/docs/specifications/data_frame_list/1.0.md).
- `data_frame`:
  [1.0](https://github.com/ArtifactDB/takane/tree/gh-pages/docs/specifications/data_frame/1.0.md).
- `dense_array`:
  [1.0](https://github.com/ArtifactDB/takane/tree/gh-pages/docs/specifications/dense_array/1.0.md).
- `fasta_file`:
  [1.0](https://github.com/ArtifactDB/takane/tree/gh-pages/docs/specifications/fasta_file/1.0.md).
- `fastq_file`:
  [1.0](https://github.com/ArtifactDB/takane/tree/gh-pages/docs/specifications/fastq_file/1.0.md).
- `genomic_ranges_list`:
  [1.0](https://github.com/ArtifactDB/takane/tree/gh-pages/docs/specifications/genomic_ranges_list/1.0.md).
- `genomic_ranges`:
  [1.0](https://github.com/ArtifactDB/takane/tree/gh-pages/docs/specifications/genomic_ranges/1.0.md).
- `gff_file`:
  [1.0](https://github.com/ArtifactDB/takane/tree/gh-pages/docs/specifications/gff_file/1.0.md).
- `gmt_file`:
  [1.0](https://github.com/ArtifactDB/takane/tree/gh-pages/docs/specifications/gmt_file/1.0.md).
- `multi_sample_dataset`:
  [1.0](https://github.com/ArtifactDB/takane/tree/gh-pages/docs/specifications/multi_sample_dataset/1.0.md).
- `ranged_summarized_experiment`:
  [1.0](https://github.com/ArtifactDB/takane/tree/gh-pages/docs/specifications/ranged_summarized_experiment/1.0.md).
- `sequence_information`:
  [1.0](https://github.com/ArtifactDB/takane/tree/gh-pages/docs/specifications/sequence_information/1.0.md).
- `sequence_string_set`:
  [1.0](https://github.com/ArtifactDB/takane/tree/gh-pages/docs/specifications/sequence_string_set/1.0.md).
- `simple_list`:
  [1.0](https://github.com/ArtifactDB/takane/tree/gh-pages/docs/specifications/simple_list/1.0.md).
- `single_cell_experiment`:
  [1.0](https://github.com/ArtifactDB/takane/tree/gh-pages/docs/specifications/single_cell_experiment/1.0.md).
- `spatial_experiment`:
  [1.0](https://github.com/ArtifactDB/takane/tree/gh-pages/docs/specifications/spatial_experiment/1.0.md).
- `string_factor`:
  [1.0](https://github.com/ArtifactDB/takane/tree/gh-pages/docs/specifications/string_factor/1.0.md).
- `summarized_experiment`:
  [1.0](https://github.com/ArtifactDB/takane/tree/gh-pages/docs/specifications/summarized_experiment/1.0.md).
- `vcf_experiment`:
  [1.0](https://github.com/ArtifactDB/takane/tree/gh-pages/docs/specifications/vcf_experiment/1.0.md).

## Validation

The `takane::validate()` function inspects the object's directory and validates its contents, throwing an error if the contents are not valid.

```cpp
#include "takane/takane.hpp"

takane::validate(dir);
```

The idea is to bind to the **takane** library in application-specific frameworks, e.g., via R/Python's foreign function interfaces.
This consistently enforces the format expectations for each object, regardless of how the saving was performed by each application.
For example, we might use the [**alabaster**](https://github.com/ArtifactDB/alabaster.base) framework to save Bioconductor objects to disk:

```r
library(alabaster.base)
tmp <- tempfile()
df <- DataFrame(X=1:10, Y=letters[1:10])
saveObject(df, tmp)
validateObject(tmp) # calls takane::validate()
```

If the validation passes, we can be confident that the same object can be reconstructed in different frameworks, 
e.g., with [**dolomite**](https://github.com/ArtifactDB/dolomite-base) packages in Python.

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
Not really sure why I picked Takane but she's nice enough.

![Takane GIF](https://64.media.tumblr.com/17ecbb29ab7ed3328ed854c1b02e0eec/tumblr_o49c7i4jUu1th93f0o1_540.gif)
