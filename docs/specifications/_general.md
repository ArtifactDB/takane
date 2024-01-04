# General comments

## Object directories

Every object is represented by a directory on disk.
All files related to the object's format are stored within this directory.
This allows each object format to specify any number of arbitrarily named files without interfering with other objects in different directories. 

## Object metadata

All object directories should contain an `OBJECT` file.
This is a JSON-formatted file that contains a JSON object with the `type` property.
This is a string that specifies the type of the object, e.g., `atomic_vector`, `dense_array`.
In addition, each format typically includes extra format-specific metadata in another property named after the object type.
For example, for the [`summarized_experiment` format](summarized_experiment):

```json
{
    "type": "summarized_experiment",
    "summarized_experiment": {
        "version": "1.0",
        "dimensions": [ 10, 20 }
    }
}
```

This allows formats to store small pieces of information that don't easily fit into other files.
By nesting it inside a `summarized_experiment` property, we ensure that these properties do not conflict with those of other derived formats.
For example, the [`ranged_summarized_experiment` format](ranged_summarized_experiment) is built on top of the `summarized_experiment`, but has its own version string:

```json
{
    "type": "ranged_summarized_experiment",
    "summarized_experiment": {
        "version": "1.0",
        "dimensions": [ 10, 20 }
    },
    "ranged_summarized_experiment": {
        "version": "1.0"
    }
}
```

## Child objects

Some objects are composed of other objects, e.g., a summarized experiment is composed of several data frames and assays.
A format for a composed object may hold these "child" objects in subdirectories of the composed object's directory.
This allows developers to easily re-use child specifications to construct a format for a complex object.

## Height and dimensions

Some objects may have a concept of "height" (i.e., the vertical length) and "dimensions".
These concepts are typically used in complex format specifications to ensure that the child objects have the correct shape.
For example, a summarized experiment expects that all assays have the same first two dimensions, 
and that the data frames for the row and column annotations are equal to the number of rows and columns, respectively.
These expectations are enforced by comparing each child object's height/dimensions to that of the summarized experiment.

