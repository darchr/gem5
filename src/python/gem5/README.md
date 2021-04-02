# gem5 Python interface

This directory holds all of the new gem5 Python packages.

To use the new python interface ...?

## Setting up editors/IDEs

If you use VS Code...

## Sub-packages in the gem5 namespace

### simulation

This package contains functions and objects useful for writing scripts to run and control gem5.

### components

These are the *actualized* gem5 models ready to be included in configuration scripts.
All of the components in this package are built from adding parameter to the gem5 models in the internal package.

### stats

### util

### internal

This is the internal interface between the C++ code in gem5 and these Python bindings.
This is an internal interface and can change at any time.
All users should use other packages to access these internal implementations.

For historical reasons, all internal objects are exposed through the `_m5` module.

## Developing the gem5 python library

We have chosen to include this namespace *in the gem5 binary*.
Thus, like with all changes in `src/`, you have to rebuild gem5 for your changes to this code to take effect.

However, if you want to avoid rebuilding while testing you can use the `M5_OVERRIDE_PY_SOURCE=True` environment variable.
**This should only be used when testing** in special circumstances!

### Adding a new python file

Whenever adding a new file that you want included in the `gem5` python namespace, you must declare it in a `SConscript` file with `PySource(<module name>, <file path>)`.
`PySource()` takes two parameters, the module name for the file and the path of the file to add.

### Python file marshalling

When running in gem5's python interpreter, when you run `import` it will use the importer object `CodeImporter` in `src/python/importer.py`.
This file is executed during the startup of gem5 in `main.cc` by calling `EmbeddedPython::initAll()`.
This function calls `PyImport_ExecCodeModule` on the `importer` module.

Then, all of the embedded python files are added with the `add_module` function from the `importer` module.
This happens because every Python module's C++ marshal file has an instantiation of an `EmbeddedPython` object.
In the `EmbeddedPython` constructor, every python file is added to a list of modules to add to the importer, as described above.

To create all of these `EmbeddedPython` objects, the `SConscript` in `src/` creates the `build/.../python/m5/<module>.py.cc` file which has the compiled bytecode and the instantiation of the `EmbeddedPython` object.
To add a new python module to be "embedded" you need to create a `PySource` instance in the SConscript file.
