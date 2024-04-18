# ResData
Python package for reading and writing the result files from
reservoir simulators. The file types covered are the
RESTART, INIT, RFT, Summary and GRID files in unified and non-unified, and formatted and unformatted.

ResData officially only supports Linux and macOS. It was initially developed as part of the
[_ert_](http://github.com/Equinor/ert) project.

## Using
[ResData is available on PyPI](https://pypi.org/project/resdata/) and can be installed into a [Python virtual environment](https://docs.python.org/3/library/venv.html#creating-virtual-environments) with `pip`:

```sh
pip install resdata
```

## Building
ResData is a Python project with a C++ extension layer. Most of the functionality is implemented in C++ and uses [cwrap](https://github.com/equinor/cwrap) for binding it to Python.

A C++17-compatible compiler, like GCC 8+ or Clang 11+ is required. Other C++ dependencies are brought in automatically by [Conan](https://conan.io) during [CMake](https://cmake.org) compilation.

In a [Python virtual environment](https://docs.python.org/3/library/venv.html#creating-virtual-environments), run:
```sh
# Fetch directly from GitHub
pip install git+https://github.com/equinor/resdata

# If git-cloned, install local directory in editable mode
pip install --editable .
```

## Running tests
As this codebase contains both Python and C++ code, there are tests for both Python and C++.

### Python tests
These tests use [pytest](https://pytest.org) and require that ResData is installed into a Python virtualenv in `--editable` mode, as described in the [Building](#Building) section.

Ensure that pytest is installed and do the following to 
```sh
# Install pytest
pip install pytest

# Run all tests in the python/tests directory
pytest python/tests
```

### C++ tests
ResData uses a homegrown testing suite as well as [Catch2, 2.x](https://github.com/catchorg/Catch2) which is compiled via CMake and ran using `ctest`.

Ensure that `cmake` and `conan` version 1 is installed.

```sh
# Generate CMake build files into `build/`
cmake -B build .

# Build project
cmake --build build

# Run all tests
ctest --test-dir build
```
