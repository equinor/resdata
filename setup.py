import os
import re
import sys
import platform
import subprocess
import pathlib

from setuptools import setup, Extension, find_packages
from setuptools.command.build_ext import build_ext
from distutils.version import LooseVersion


class CMakeExtension(Extension):
    def __init__(self, name, sourcedir=''):
        Extension.__init__(self, name, sources=[])
        self.sourcedir = os.path.abspath(sourcedir)


class CMakeBuild(build_ext):
    def run(self):
        try:
            out = subprocess.check_output(['cmake', '--version'])
        except OSError:
            raise RuntimeError("CMake must be installed to build the following extensions: " +
                               ", ".join(e.name for e in self.extensions))

        for ext in self.extensions:
            self.build_extension(ext)

    def build_extension(self, ext):
        print(ext)
        extdir = os.path.abspath(os.path.dirname(self.get_ext_fullpath(ext.name)))
        self.announce(extdir)
        print(extdir)
        extdir1 = pathlib.Path(self.get_ext_fullpath(ext.name))
        print(str(extdir1.parent.absolute()))
        print(self.libraries)
        libdir = extdir
        #libdir =  os.path.join(extdir, 'ecl')
        self.library_dirs.append(libdir)
        print(self.library_dirs)
        self.rpath.append(libdir)
        print(self.rpath)
        cmake_args = ['-DCMAKE_LIBRARY_OUTPUT_DIRECTORY=' + libdir]
        cfg = 'Debug' if self.debug else 'Release'
        build_args = ['--config', cfg]

        cmake_args += ['-DCMAKE_BUILD_TYPE=' + cfg]
        build_args += ['--', '-j2']

        env = os.environ.copy()
        env['CXXFLAGS'] = '{} -DVERSION_INFO=\\"{}\\"'.format(env.get('CXXFLAGS', ''),
                                                              self.distribution.get_version())
        if not os.path.exists(self.build_temp):
            os.makedirs(self.build_temp)
        print(cmake_args)
        subprocess.check_call(['cmake', ext.sourcedir] + cmake_args, cwd=self.build_temp, env=env)
        print(build_args)
        subprocess.check_call(['cmake', '--build', '.'] + build_args, cwd=self.build_temp)

setup(
    name='opm_libecl',
    version='0.1',
    author='OPM',
    author_email='chandan.nath@gmail.com',
    description='libecl',
    long_description='',
    packages=find_packages(where='python', exclude=["*.tests", "*.tests.*", "tests.*", "tests"]),
    package_dir={'': 'python'},
    ext_package='ecl',
    ext_modules=[CMakeExtension('opm_libecl')],
    cmdclass=dict(build_ext=CMakeBuild),
    zip_safe=False,
)
