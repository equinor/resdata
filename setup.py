import platform
import sys
from pathlib import Path

import setuptools
import skbuild
from setuptools_scm import get_version

from _custom_build.conan import prepare_conan

version = get_version(relative_to=__file__, write_to="python/resdata/version.py")


def get_skbuild_dir():
    python_version = f"{sys.version_info.major}.{sys.version_info.minor}"
    return Path(
        "_skbuild",
        f"{platform.system().lower()}-{platform.machine()}-{python_version}",
        "cmake-build",
    )


CMAKE_ARGS_FROM_PRESET = prepare_conan(get_skbuild_dir())

with open("README.md") as f:
    long_description = f.read()


def utility_wrappers():
    """
    Wrappers around resdata's "application" utilities. These are only supported on
    Linux at this time so only create the wrapper when on Linux.
    """
    if sys.platform != "linux":
        return []

    return [
        name + " = resdata.bin:main"
        for name in (
            "rd_pack.x",
            "rd_unpack.x",
        )
    ] + ["summary.x = view_summary.__main__:main"]


skbuild.setup(
    name="resdata",
    long_description=long_description,
    long_description_content_type="text/markdown",
    url="https://github.com/equinor/resdata",
    packages=setuptools.find_packages(
        where="python",
        exclude=["*.tests", "*.tests.*", "tests.*", "tests", "ert.*", "ert"],
    ),
    package_dir={"": "python"},
    package_data={"resdata": ["py.typed", "well/*.pyi", "resfile/*.pyi"]},
    platforms="any",
    install_requires=[
        "cwrap",
        "numpy",
        "pandas",
        "python-dateutil",
        "natsort",
        "typing_extensions",
        "resfo-utilities>=0.4.0",
    ],
    setup_requires=["conan>=2"],
    entry_points={"console_scripts": utility_wrappers()},
    cmake_args=CMAKE_ARGS_FROM_PRESET
    + [
        "-DRD_VERSION=" + version,
        "-DBUILD_APPLICATIONS=" + ("ON" if sys.platform == "linux" else "OFF"),
        "-DBUILD_TESTS=OFF",
        "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON",
        "-DCMAKE_INSTALL_BINDIR=python/resdata/.bin",
        "-DCMAKE_INSTALL_LIBDIR=python/resdata/.libs",
        "-DCMAKE_INSTALL_INCLUDEDIR=python/resdata/.include",
    ],
    classifiers=[
        "Development Status :: 5 - Production/Stable",
        "Environment :: Other Environment",
        "Intended Audience :: Developers",
        "Intended Audience :: Science/Research",
        "Natural Language :: English",
        "Programming Language :: Python",
        "Programming Language :: Python :: 3.11",
        "Programming Language :: Python :: 3.12",
        "Programming Language :: Python :: 3.13",
        "Programming Language :: Python :: 3.14",
        "Topic :: Scientific/Engineering",
        "Topic :: Scientific/Engineering :: Physics",
        "Topic :: Software Development :: Libraries",
        "Topic :: Utilities",
    ],
    version=version,
)
