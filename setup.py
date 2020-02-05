#!/usr/bin/env python

import os
import skbuild
import setuptools
from setuptools_scm import get_version


version = get_version(relative_to=__file__, write_to="python/ecl/version.py")

with open("README.md") as f:
    long_description = f.read()

skbuild.setup(
    name="libecl",
    author="Equinor ASA",
    description="Package for reading and writing the result files from the ECLIPSE reservoir simulator",
    long_description=long_description,
    long_description_content_type="text/markdown",
    url="https://github.com/equinor/libecl",
    packages=setuptools.find_packages(where='python', exclude=["*.tests", "*.tests.*", "tests.*", "tests"]),
    package_dir={"": "python"},
    license="GPL-3.0",
    platforms="any",
    install_requires=[
        "cwrap",
        "functools32;python_version=='2.7'",
        "future",
        "numpy;python_version>='3.0'",
        "numpy<=1.16.6;python_version=='2.7'",
        "pandas;python_version>='3.0'",
        "pandas<=0.25.3;python_version=='2.7'",
        "six"
    ],
    tests_require=["pytest"],
    cmake_args=[
        "-DECL_VERSION=" + version,
        "-DCMAKE_INSTALL_LIBDIR=python/ecl/.libs",
        "-DCMAKE_INSTALL_INCLUDEDIR=python/ecl/.include",
        # we can safely pass OSX_DEPLOYMENT_TARGET as it's ignored on
        # everything not OS X. We depend on C++11, which makes our minimum
        # supported OS X release 10.9
        "-DCMAKE_OSX_DEPLOYMENT_TARGET=10.9"
    ],
    # skbuild's test imples develop, which is pretty obnoxious instead, use a
    # manually integrated pytest.
    cmdclass={"test": setuptools.command.test.test},
    classifiers=[
        "Development Status :: 5 - Production/Stable",
        "Environment :: Other Environment",
        "Intended Audience :: Developers",
        "Intended Audience :: Science/Research",
        "License :: OSI Approved :: GNU General Public License v3 (GPLv3)",
        "Natural Language :: English",
        "Programming Language :: Python",
        "Programming Language :: Python :: 2.7",
        "Programming Language :: Python :: 3.5",
        "Programming Language :: Python :: 3.6",
        "Topic :: Scientific/Engineering",
        "Topic :: Scientific/Engineering :: Physics",
        "Topic :: Software Development :: Libraries",
        "Topic :: Utilities"
    ],
    version=version
)
