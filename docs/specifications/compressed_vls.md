# Compressed variable-length strings

## Overview

Some HDF5-based object specifications support the compressed variable length string (CVLS) format, e.g., [`atomic_vector`](atomic_vector).
This format addresses HDF5's inability to efficiently handle arrays of variable-length strings.
Using a fixed-length datatype requires padding all strings to the longest string, causing an increase in disk usage that cannot be completely offset by compression;
while HDF5's own variable-length string datatype does not compress the strings themselves, only the pointers to those strings.
CVLS stores variable-length strings directly without any padding, and the dataset can be chunked and compressed to improve storage efficiency.

## Specification

The CVLS format consists of a HDF5 group, denoted here as a `**/` placeholder.
(Each specification should replace `**/` below with the actual name of the group containing the CVLS-formatted data.)
This group should contain: 

- `pointers`, a dataset using a compound datatype consisting of 2 members named `offset` and `length`.
  Each member should be of a datatype that can be represented by an unsigned 64-bit integer.
  The expected dimensionality of this dataset should be declared by each specification.
- `heap`, a 1-dimensional dataset of unsigned 8-bit integers.
  This contains bytes from all of the strings in the array, e.g., by concatenating all strings.

The $i$-th entry of the `**/pointers` dataset refers to a slice $[o_i, o_i + l_i)$ of the `**/heap` dataset,
where $o_i$ is the zero-based offset and $l_i$ is the length in bytes from the `offset` and `length` members, respectively.
This slice defines a variable-length UTF-8-encoded string, terminated at the first null in the slice.
If no null character is present within the slice, the string consists of the entire slice. 

Entries of the `**/pointers` dataset may be sorted in any order and may refer to overlapping or non-contiguous slices.
The only requirement is that each $[o_i, o_i + l_i)$ slice lies within the boundaries of the heap dataset.

No restrictions are placed on the contents of the `**/heap` dataset, as long as the strings can be recovered from slices as described above.
Writers of the `**/heap` dataset may concatenate strings in arbitrary order, merge duplicate strings, add padding between strings, etc.
The heap may contain unused bytes, i.e., not referenced by any slice in the `**/pointers` dataset.

Optionally, the `**/pointers` dataset may contain a `missing-value-placeholder` scalar string attribute.
If this attribute is present, any strings in the CVLS array that are equal to the placeholder should be treated as missing.

## Usage instructions

Some care is required to avoid inadvertent casts to/from a `char` of platform-specific signedness to the heap's unsigned integer type.
Recall that both ASCII and UTF-8 define a character in terms of its binary representation, e.g., `0b01000001` for `A`.
Thus, implementations should ensure that the binary representation of each character is preserved when reading or writing to the heap. 
We recommend following these procedures:

- For reading, we first read the bytes into an `uint8_t` array via `H5::DataSet::read()` with `NATIVE_UINT8`.
  We then access those bytes via an aliased `char*` to extract each VLS.
- For writing, we create an aliased `char*` pointer to a `uint8_t` array and copy each VLS into this array via the aliased pointer.
  We then use `H5::DataSet::write()` with `NATIVE_UINT8` to write the `uint8_t` array. 

This approach ensures that each byte is faithfully transferred between the HDF5 file and memory as a `uint8_t` with a well-defined and portable binary representation.
The aliasing pointer then interprets the byte as a `char` with the same sequence of bits, regardless of the platform's `char` signedness.

When writing a CVLS array, we recommend that entries in the same chunk of the pointer dataset refer to neighboring parts of the heap dataset.
This allows readers to efficiently extract bytes from the latter when iterating through chunks of the former.
