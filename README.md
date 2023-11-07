# resdata [![testing](https://github.com/equinor/resdata/actions/workflows/testing.yml/badge.svg)](https://github.com/equinor/resdata/actions/workflows/testing.yml)


*resdata* is a package for reading and writing the result files from
the reservoir simulators. The file types covered are the
restart, init, rft, summary and grid files. Both unified and
non-unified and formatted and unformatted files are supported.

*resdata* is mainly developed on *Linux* and *macOS*, in addition there
is a portability layer which ensures that most of the functionality is
available on *Windows*. The main functionality is written in C/C++, and
should typically be linked in in other compiled programs. *resdata* was
initially developed as part of the [Ensemble Reservoir
Tool](http://github.com/Equinor/ert), other applications using
*resdata* are the reservoir simulator flow and Resinsight from the [OPM
project](http://github.com/OPM/).

### Dependencies

Regardless of how you build *resdata*, it will depend on the following system-level
components.

| Software                                           | Debian / Ubuntu | RHEL / Fedora | macOS                |
|----------------------------------------------------|-----------------|---------------|----------------------|
| `libz`                                             | `zlib1g-dev`    | `zlib-devel`  | _builtin_            |
| [Conan](https://conan.io)                          | N/A             | N/A           | `conan` _(Homebrew)_ |
| [pipx](https://pypi.org/project/pipx) _(Optional)_ | `pipx`          | `pipx`        | `pipx` _(Homebrew)_  |

Note: The Conan package manager is not available for most Linux systems. Conan
recommends installing it via `pip`. If using `pipx`, simply `pipx install conan`
and it'll be availabe for your user regardless if you're using a virtualenv or
not.

### Alternative 1: Python only ###
For small interactive scripts, such as forward models, the recommended way to
use *resdata* is by installing it from PyPI. This method doesn't require setting
`PYTHONPATH` or `LD_LIBRARY_PATH` environment variables:

```
$ pip install resdata
```

### Alternative 2: C library only ###
This is for when you need to link directly with the *resdata* C library, but
don't need the Python bindings. *resdata* requires a conforming C++11 or later
compiler such as GNU GCC, the CMake build system and, optionally, zlib.

```bash
$ git clone https://github.com/Equinor/resdata
$ mkdir resdata/build
$ cd resdata/build
$ cmake ..
$ make
$ make install
```

To install *resdata* in a non-standard location, add
`-DCMAKE_INSTALL_PREFIX=/path/to/install` to the first `cmake` command. Remember
to set `LD_LIBRARY_PATH=/path/to/install/lib64:$LD_LIBRARY_PATH` if you do use a
non-standard location for your program to find `resdata.so`.

If you intend to develop and change *resdata* you should build the tests by
passing `-DBUILD_TESTS=ON` and run the tests with `ctest`.

### Alternative 3: C library with Python bindings ###
It is also possible to install both the C library and Python bindings using
CMake. Note that this alternative is incompatible with *resdata* installed from
PyPI (_Alternative 1_). As before, we require a conforming C++11 or later
compiler, CMake and, optionally, zlib.

```bash
$ git clone https://github.com/Equinor/resdata
$ mkdir resdata/build
$ cd resdata/build
$ pip install -r ../requirements.txt
$ cmake .. -DENABLE_PYTHON=ON
$ make
$ make install
```

You will most likely want to install *resdata* into a Python virtual environment.
First activate the virtualenv, then add the argument
`-DCMAKE_INSTALL_PREFIX=$(python -c "import sys; print(sys.prefix)")` to the
`cmake` command when building.

Then, you must tell Python where to find the package[1]:

```bash
$ export PYTHONPATH=/path/to/install/lib/python2.7/site-packages:$PYTHONPATH
$ export LD_LIBRARY_PATH=/path/to/install/lib64:$LD_LIBRARY_PATH
```

Then you can fire up your Python interpreter and try it out:
```python
>>> from resdata.summary import Summary
>>> import sys

>>> summary = Summary(sys.argv[1])
>>> fopt = summary.numpy_vector("FOPT")
```

The installation with Python enabled is described in a [YouTube video](https://www.youtube.com/watch?v=Qqy1vA1PSk8) by Carl Fredrik Berg.

[1]: The exact paths here will depend on your system and Python version. The example given is for a RedHat system with Python version 2.7.
