 #### Basic

The `partitions.h5` file should contain a `bumpy_atomic_array` group.

The `bumpy_atomic_array` group should contain `dimensions`, a 1-dimensional dataset of any type that is representable by a 64-bit unsigned integer.
This contains the extents of the dimensions of the bumpy atomic array.

The `bumpy_atomic_array` group should contain `lengths`, a 1-dimensional dataset of any type that is representable by a 64-bit unsigned integer.
Each entry of this dataset corresponds to an entry of the bumpy atomic array (i.e., a single atomic vector),
and defines the number of consecutive elements in the `concatenated` object that should be used to construct each atomic vector.

#### Dense array 

If the `bumpy_atomic_array` group does not contain an `indices` group, it is assumed that the bumpy atomic array is dense.

The `bumpy_atomic_array/lengths` dataset should have length equal to the product of the entries of the `bumpy_atomic_array/dimensions` dataset.

Each entry of the bumpy atomic array maps to an entry of the `bumpy_atomic_array/lengths` dataset by flattening the former to a 1-dimensional vector.
The first dimension of the bumpy atomic array is the fastest-changing, then the second, and so on.
For example, given a bumpy atomic array of dimensions $(X, Y, Z)$, array entry $(x, y, z)$ would map to entry $x + X(y + Yz)$ of the `bumpy_atomic_array/lengths` dataset. 

#### Sparse array

If the `bumpy_atomic_array` group contains an `indices` group, it is assumed that the bumpy atomic array is sparse.

The `bumpy_atomic_array/indices` group should contain one dataset per dimension.
Specifically, the $d$-th dimension should be represented by the `bumpy_atomic_array/indices/{d}` dataset.
Each dataset should be a 1-dimensional dataset of length equal to that of the `bumpy_atomic_array/lengths` dataset.
The datatype should be representable by a 64-bit unsigned integer.

The `bumpy_atomic_array/indices/d` datasets contain the sparse array coordinates for each entry of the `bumpy_atomic_array/lengths` dataset.
Given a bumpy matrix of dimensionality $D$, the $i$-th entry of `bumpy_atomic_array/lengths` would map to the sparse array coordinates $(x_{i0}, x_{i1}, ..., x_{iD})$,
where $x_{id}$ is the $i$-th entry of the `bumpy_atomic_array/indices/d` dataset.
Entries in each `bumpy_atomic_array/indices/d` dataset should be less than the extent of the $d$-th dimension.

Sparse array coordinates should be sorted in increasing "reverse-lexicographical" order.
That is, we expect $x_{id} < x_{(i+1)d}$ for at least one $d$ where $x_{if} = x_{(i + 1)f}$ for all $f \in (d+1, ..., D)$.
In other words, the first dimension should contain the fastest-changing coordinates, then the second, and so on.
There should also be no duplicate coordinates.

Aside from the aforementioned datasets, no other objects should be present in the `bumpy_atomic_array/indices` group.

#### Names

Optionally, the `bumpy_atomic_array` group may contain a `names` group.
This may contain any number of 1-dimensional string datasets named after the corresponding dimension of the bumpy array,
i.e., the `bumpy_atomic_array/names/{d}` dataset corresponds to the $d$-th dimension.
If present, each dataset contains the names for its dimension and should have a length equal to the extent of that dimension.

Aside from the aforementioned datasets, no other objects should be present in the `bumpy_atomic_array/names` group.
