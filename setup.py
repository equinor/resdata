#!/usr/bin/env python
from setuptools import setup, Extension

setup(name='libecl',
      version='1.1',
      description='Utilities for reading and writing Eclipse binary files',
      author='Joakim Hove',
      author_email='joakim.hove@gmail.com',
      url='https://github.com/Statoil/libecl',

      package_dir = {"" : "python"},
      packages=["ecl",
                "ecl.util", "ecl.util.enums",
                "ecl.ecl", "ecl.ecl.faults",
                "ecl.geo",
                "ecl.well",
                "ecl.test"],

      ext_modules = [ Extension("ecl/_ecl",
                                sources = ["python/c_inter/ecl.cpp"],
                                #extra_link_args=["-Wl,--whole-archive","-lecl","-Wl,--no-whole-archive"],
                                libraries = ["ecl","lapack","blas","z","m"],
                                include_dirs=["/private/joaho/work/ERT/libecl/lib/include"],
                                library_dirs= ["/private/joaho/work/ERT/install/lib64"])
                      ],
      test_suite = 'tests'
)
