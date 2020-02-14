# libecl [![Build Status](https://travis-ci.org/Equinor/libecl.svg?branch=master)](https://travis-ci.org/Equinor/libecl)


*libecl* is a package for reading and writing the result files from
the Eclipse reservoir simulator. The file types covered are the
restart, init, rft, summary and grid files. Both unified and
non-unified and formatted and unformatted files are supported.

*libecl* is mainly developed on *Linux* and *macOS*, in addition there
is a portability layer which ensures that most of the functionality is
available on *Windows*. The main functionality is written in C/C++, and
should typically be linked in in other compiled programs. *libecl* was
initially developed as part of the [Ensemble Reservoir
Tool](http://github.com/Equinor/ert), other applications using
*libecl* are the reservoir simulator flow and Resinsight from the [OPM
project](http://github.com/OPM/).

### Alternative 1: Python only ###
For small interactive scripts, such as forward models, the recommended way to
use *libecl* is by installing it from PyPI. This method doesn't require setting
`PYTHONPATH` or `LD_LIBRARY_PATH` environment variables:

```
$ pip install libecl
```

### Alternative 2: C library only ###
This is for when you need to link directly with the *libecl* C library, but
don't need the Python bindings. *libecl* requires a conforming C++11 or later
compiler such as GNU GCC, the CMake build system and, optionally, zlib.

```bash
$ git clone https://github.com/Equinor/libecl
$ cmake libecl
$ cmake --build libecl --target install
```

To install *libecl* in a non-standard location, add
`-DCMAKE_INSTALL_PATH=/path/to/install` to the first `cmake` command. If you
intend to develop and change *libecl* you should build the tests by passing
`-DBUILD_TESTS=ON` and run the tests with `ctest`.

### Alternative 3: C library with Python bindings ###
It is also possible to install both the C library and Python bindings using
CMake. Note that this alternative is incompatible with *libecl* installed from
PyPI (_Alternative 1_). As before, we require a conforming C++11 or later
compiler, CMake and, optionally, zlib.

```bash
$ git clone https://github.com/Equinor/libecl
$ pip install -r libecl/requirements.txt
$ cmake libecl -DENABLE_PYTHON=ON
$ cmake --build libecl --target install
```

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
