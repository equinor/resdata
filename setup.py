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
    packages=setuptools.find_packages(
        where="python",
        exclude=["*.tests", "*.tests.*", "tests.*", "tests", "ert.*", "ert"],
    ),
    package_dir={"": "python"},
    package_data={"resdata": ["py.typed", "well/*.pyi", "resfile/*.pyi"]},
    platforms="any",
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
    version=version,
)
