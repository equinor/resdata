# ecl [![Build Status](https://github.com/equinor/libecl/workflows/ecl%20testing/badge.svg)](https://github.com/equinor/libecl/actions)


*ecl* is a package for reading and writing the result files from
the Eclipse reservoir simulator. The file types covered are the
restart, init, rft, summary and grid files. Both unified and
non-unified and formatted and unformatted files are supported.

*ecl* is mainly developed on *Linux* and *macOS*, in addition there
is a portability layer which ensures that most of the functionality is
available on *Windows*. The main functionality is written in C/C++, and
should typically be linked in in other compiled programs. *ecl* was
initially developed as part of the [Ensemble Reservoir
Tool](http://github.com/Equinor/ert), other applications using
*ecl* are the reservoir simulator flow and Resinsight from the [OPM
project](http://github.com/OPM/).

### Dependencies

Regardless of how you build *ecl*, it will depend on the following system-level
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
use *ecl* is by installing it from PyPI. This method doesn't require setting
`PYTHONPATH` or `LD_LIBRARY_PATH` environment variables:

```
$ pip install ecl
```

### Alternative 2: C library only ###
This is for when you need to link directly with the *ecl* C library, but
don't need the Python bindings. *ecl* requires a conforming C++11 or later
compiler such as GNU GCC, the CMake build system and, optionally, zlib.

```bash
$ git clone https://github.com/Equinor/ecl
$ mkdir ecl/build
$ cd ecl/build
$ cmake ..
$ make
$ make install
```

To install *ecl* in a non-standard location, add
`-DCMAKE_INSTALL_PREFIX=/path/to/install` to the first `cmake` command. Remember
to set `LD_LIBRARY_PATH=/path/to/install/lib64:$LD_LIBRARY_PATH` if you do use a
non-standard location for your program to find `libecl.so`.

If you intend to develop and change *ecl* you should build the tests by
passing `-DBUILD_TESTS=ON` and run the tests with `ctest`.

### Alternative 3: C library with Python bindings ###
It is also possible to install both the C library and Python bindings using
CMake. Note that this alternative is incompatible with *ecl* installed from
PyPI (_Alternative 1_). As before, we require a conforming C++11 or later
compiler, CMake and, optionally, zlib.

```bash
$ git clone https://github.com/Equinor/ecl
$ mkdir ecl/build
$ cd ecl/build
$ pip install -r ../requirements.txt
$ cmake .. -DENABLE_PYTHON=ON
$ make
$ make install
```

You will most likely want to install *ecl* into a Python virtual environment.
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
>>> from ecl.summary import EclSum
>>> import sys

>>> summary = EclSum(sys.argv[1])
>>> fopt = summary.numpy_vector("FOPT")
```

The installation with Python enabled is described in a [YouTube video](https://www.youtube.com/watch?v=Qqy1vA1PSk8) by Carl Fredrik Berg.

[1]: The exact paths here will depend on your system and Python version. The example given is for a RedHat system with Python version 2.7.
