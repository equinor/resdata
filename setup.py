import os
import sys

import setuptools
import skbuild
from setuptools_scm import get_version

version = get_version(relative_to=__file__, write_to="python/resdata/version.py")


# Corporate networks tend to be behind a proxy server with their own non-public
# SSL certificates. Conan keeps its own certificates, whose path we can override
if "CONAN_CACERT_PATH" not in os.environ:
    # Look for a RHEL-compatible system-wide file
    for file_ in ("/etc/pki/tls/cert.pem",):
        if not os.path.isfile(file_):
            continue
        os.environ["CONAN_CACERT_PATH"] = file_
        break


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
            "CF_dump",
            "convert.x",
            "rd_pack.x",
            "rd_unpack.x",
            "grdecl_grid",
            "grdecl_test.x",
            "grid_dump.x",
            "grid_dump_ascii.x",
            "grid_info.x",
            "kw_extract",
            "kw_list.x",
            "load_test.x",
            "make_grid",
            "ri_well_test",
            "segment_info",
            "select_test.x",
            "summary.x",
        )
    ]


skbuild.setup(
    name="resdata",
    author="Equinor ASA",
    author_email="fg_sib-scout@equinor.com",
    description="Package for reading and writing the fortran result files from reservoir simulators",
    long_description=long_description,
    long_description_content_type="text/markdown",
    url="https://github.com/equinor/resdata",
    packages=setuptools.find_packages(
        where="python",
        exclude=["*.tests", "*.tests.*", "tests.*", "tests", "ert.*", "ert"],
    ),
    package_dir={"": "python"},
    license="GPL-3.0",
    platforms="any",
    install_requires=[
        "cwrap",
        "numpy",
        "pandas",
    ],
    setup_requires=["conan<2"],
    entry_points={"console_scripts": utility_wrappers()},
    cmake_args=[
        "-DRD_VERSION=" + version,
        "-DBUILD_APPLICATIONS=" + ("ON" if sys.platform == "linux" else "OFF"),
        "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON",
        "-DCMAKE_INSTALL_BINDIR=python/resdata/.bin",
        "-DCMAKE_INSTALL_LIBDIR=python/resdata/.libs",
        "-DCMAKE_INSTALL_INCLUDEDIR=python/resdata/.include",
        # we can safely pass OSX_DEPLOYMENT_TARGET as it's ignored on
        # everything not OS X. We depend on C++11, which makes our minimum
        # supported OS X release 10.9
        "-DCMAKE_OSX_DEPLOYMENT_TARGET=10.9",
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
        "Programming Language :: Python :: 3.8",
        "Programming Language :: Python :: 3.9",
        "Programming Language :: Python :: 3.10",
        "Programming Language :: Python :: 3.11",
        "Programming Language :: Python :: 3.12",
        "Topic :: Scientific/Engineering",
        "Topic :: Scientific/Engineering :: Physics",
        "Topic :: Software Development :: Libraries",
        "Topic :: Utilities",
    ],
    version=version,
)
