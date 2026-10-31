The `partitions.h5` file should contain an `atomic_vector_list` group.

The `atomic_vector_list` group should contain a `lengths` dataset.
This dataset should be 1-dimensional and use a datatype that is representable by a 64-bit unsigned integer.
Each entry corresponds to an element of the atomic vector list (i.e., a single atomic vector),
and defines the number of consecutive elements in the `concatenated/` object that can be used to construct each atomic vector.

Optionally, the `atomic_vector_list` group may contain `names`, a 1-dimensional string dataset of the same extent as `atomic_vector_list/lengths`.
The $i$-th entry of this dataset contains the name of the $i$-th entry of the atomic vector list.
