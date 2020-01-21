import skbuild
import setuptools


skbuild.setup(
    name='libecl',
    author='Equinor ASA',
    description='libecl',
    long_description=open("README.md", "r").read(),
    long_description_content_type="text/markdown",
    url="https://github.com/equinor/libecl",
    license="GNU General Public License, Version 3, 29 June 2007",
    packages=setuptools.find_packages(where='python', exclude=["*.tests", "*.tests.*", "tests.*", "tests"]),
    package_dir={'': 'python'},
    install_requires=[
        'cwrap',
        'numpy',
        'pandas',
    ],
    setup_requires=[
        'setuptools >= 28',
        'setuptools_scm',
        'pytest-runner'
    ],
    tests_require=[
        'pytest'
    ],
    cmake_args=[
        '-DENABLE_PYTHON_MODULE=ON',
        '-DCMAKE_OSX_DEPLOYMENT_TARGET=10.9'
    ],
    cmdclass={'test': setuptools.command.test.test},
    use_scm_version=True
)
