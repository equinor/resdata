# Compiling and Installing **_resdata_** on Windows

## Prerequisites:
* Python 3.11 or newer https://www.python.org/ or https://anaconda.org/
* Microsoft Visual Studio  https://visualstudio.microsoft.com/downloads/
* [uv](https://docs.astral.sh/uv/getting-started/installation/), used to install and manage build dependencies
* Local copy of **_resdata_**

## Instructions:
1. Download or clone the **_resdata_** GitHub repository to your local disk.

2. Python 3.11 or newer installation
   - Download a python installation or a python environment solution such as Anaconda.

3.  Download and install Microsoft Visual Studio . At a minimum **_resdata_** requires the VS Studio packages for cmake, msbuild, c and c++ compilers (CL.exe).

4. Open a MSVC command prompt such as _x64 Native Tools Command Prompt for VS 2017_ from your start menu. In the open prompt, navigate to the **_resdata_** source directory you created in step 1. Use [uv](https://docs.astral.sh/uv/) (see [installing uv](https://docs.astral.sh/uv/getting-started/installation/) if you don't already have it) to install **_resdata_**'s pinned build requirements (cmake, ninja, cwrap, etc.) via:
   ~~~~
   uv sync --group build --no-install-project
   ~~~~
   This creates a `.venv` directory with the pinned tools. Add `.venv\Scripts` to your `PATH` so `cmake` and the other build tools are picked up, for example:
   ~~~~
   set PATH=%CD%\.venv\Scripts;%PATH%
   ~~~~
   If Python is not accessible from the prompt it may be necessary to add the Python environment location to your system path variable `PATH`.

5. Execute the build commands with the desired CMAKE parameters from `README.md`. The cmake generator can be _`NMake Makefiles`_ , _`Ninja`_ or an appropriate version of _`MSVC`_. For the available options type `cmake -G` in the MSVC command prompt.

   An example build and install is provided below where %VARIABLE% are user defined directory paths:
~~~~
    cmake -G "Visual Studio 15 2017 Win64" -DCMAKE_INSTALL_PREFIX=%INSTALLPATH% -DBUILD_SHARED_LIBS="ON"  -DENABLE_PYTHON="ON"    -DCMAKE_BUILD_TYPE="Release" %SOURCEPATH%
    cmake --build %BUILDPATH% --config Release --target install
~~~~
6. For **_resdata_** to be accessible in Python the `%INSTALLPATH%\lib\pythonX.Y\site-package` and Python subdirectories must be added to the `PATH` and `PYTHONPATH` variables. Where `pythonx.y` is the current Python version _e.g._ (`python3.11`, `python3.12` _etc._) .

8. Open a Python interactive session and run `import resdata` to check that the install and paths are now set.
