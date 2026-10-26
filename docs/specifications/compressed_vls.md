The `atomic_vector` group should contain: 

- `pointers`, SOME-DIMENSIONAL dataset using a compound datatype consisting of 2 members, `"offset"` and `"length"`.
  Each member should be of a datatype that can be represented by an unsigned 64-bit integer.
- `heap`, a 1-dimensional dataset of unsigned 8-bit integers.
  This contains bytes from all of the strings in the array, e.g., by concatenating all strings.

The $i$-th entry of the `atomic_vector/pointers` dataset refers to a slice $[o_i, o_i + l_i)$ of the `atomic_vector/heap` dataset,
where $o_i$ and $l_i$ are the values of the offset and length members, respectively.
This slice defines a variable-length UTF-8-encoded string, terminated at the first null in the slice.
If no null character is present within the slice, the string consists of the entire slice. 

Entries of the `atomic_vector/pointers` dataset may be sorted in any order and may refer to overlapping or non-contiguous slices.
The only requirement is that each $[o_i, o_i + l_i)$ slice lies within the boundaries of the heap dataset.

No restrictions are placed on the contents of the `atomic_vector/heap` dataset, as long as the strings can be recovered from slices as described above.
Writers of the `atomic_vector/heap` dataset may concatenate strings in arbitrary order, merge duplicate strings, add padding between strings, etc.
The heap may contain unused bytes, i.e., not referenced by any slice in the `atomic_vector/pointers` dataset.

Optionally, the `atomic_vector/pointers` dataset may contain a `missing-value-placeholder` scalar string attribute.
If this attribute is present, any strings in the CVLS array that are equal to the placeholder should be treated as missing.
