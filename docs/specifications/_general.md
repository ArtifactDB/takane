# General concepts

## Object directories

Every object is represented by a directory on disk.
All files related to the object are stored within this directory.
This allows each object to be represented by any number of arbitrarily named files without interfering with other objects in different directories. 

## Object metadata

All object directories should contain an `OBJECT` file.
This is a JSON-formatted file that contains a JSON object with the `type` property.
The `type` is a string that specifies the type of the object, e.g., `atomic_vector`, `dense_array`.
In addition, the `OBJECT` file typically includes extra object-specific metadata in another property named after the object type.
For example, for the [`summarized_experiment` type](summarized_experiment):

```json
{
    "type": "summarized_experiment",
    "summarized_experiment": {
        "version": "1.0",
        "dimensions": [ 10, 20 ]
    }
}
```

This allows object types to store small pieces of information that don't easily fit into other files.
By nesting it inside a `summarized_experiment` property, we ensure that these properties do not conflict with those of other derived types.
For example, the [`ranged_summarized_experiment` type](ranged_summarized_experiment) is built on top of the `summarized_experiment`, but has its own version string:

```json
{
    "type": "ranged_summarized_experiment",
    "summarized_experiment": {
        "version": "1.0",
        "dimensions": [ 10, 20 ]
    },
    "ranged_summarized_experiment": {
        "version": "1.0"
    }
}
```

## Child objects

Some objects are composed of other objects, e.g., a summarized experiment is composed of several data frames and assays.
The file representation for a composed object type should hold these "child" objects in subdirectories of the composed object's directory;
these subdirectories will then be inspected recursively by readers and validators.
By mandating that the child object is of a certain type, developers can effectively re-use child specifications in the specification for a complex object.

## Height and dimensions

Some objects may have a concept of "height" (i.e., vertical length) and "dimensions".
These concepts are typically used in complex object specifications to ensure that the child objects have the correct shape.
For example, a summarized experiment expects that all assays have the same first two dimensions, 
and that the data frames for the row and column annotations are equal to the number of rows and columns, respectively.
These expectations are enforced by comparing each child object's height/dimensions to that of the summarized experiment.

## Derived types

An object type is derived from another object type if the former satisfies all of the file requirements of the latter.
For example, the `ranged_summarized_experiment` type is derived from the `summarized_experiment` type,
storing its assays, row data and column data in the same files as specified in the `summarized_experiment` specification.
A derived type can be used in any place where an instance of the base type is expected.
This provides some flexibility in the specification of composed types, by allowing child objects to be derived from a certain base type.

## Object interfaces

An object interface describe an abstract concept of an object, relating to how the object behaves in an application rather than how it is represented on disk.
Known interfaces are listed below:

- `SIMPLE_LIST`: a list of arbitrary objects.
  The list should be indexable by position.
- `DATA_FRAME`: a columnar data store.
  Each column should have the same height, equal to the number of rows in the object.
  Columns should be named.
- `SUMMARIZED_EXPERIMENT`: a two-dimensional object containing assays, row data and column data.
  Any number of assays are allowed.
  Any object may be used as an assay, as long as it has at least 2 dimensions with extents equal to that of the object itself.
  The row and column data should satisfy the `DATA_FRAME` interface, and should have number of rows equal to the number of rows or columns, respectively, of the object itself.

Different object types that satisfy the same object interface may not have the same on-disk representation.
For example, a `vcf_experiment` satisfies the `SUMMARIZED_EXPERIMENT` interface but has a completely different on-disk representation from a `summarized_experiment`. 
The "satisfies interface" concept is effectively a more relaxed concept of the derived-base relationship,
and allows the specification for a composed object to consider a wider range of types for its child objects.
