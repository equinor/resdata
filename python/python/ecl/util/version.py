#  Copyright (C) 2017  Statoil ASA, Norway.
#
#  This file is part of ERT - Ensemble based Reservoir Tool.
#
#  ERT is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  ERT is distributed in the hope that it will be useful, but WITHOUT ANY
#  WARRANTY; without even the implied warranty of MERCHANTABILITY or
#  FITNESS FOR A PARTICULAR PURPOSE.
#
#  See the GNU General Public License at <http://www.gnu.org/licenses/gpl.html>
#  for more details.

from ecl.util import monkey_the_camel
from ecl.util import UtilPrototype


def cmp_method(method):
    def cmp_wrapper(self, other):
        if not isinstance(other, Version):
            other = Version(other[0], other[1], other[2])

        return method(self, other)

    return cmp_wrapper


class Version(object):
    _build_time = UtilPrototype("char* version_get_build_time()")
    _git_commit = UtilPrototype("char* version_get_git_commit()")
    _git_commit_short = UtilPrototype("char* version_get_git_commit_short()")
    _major_version = UtilPrototype("int version_get_major_ert_version()")
    _minor_version = UtilPrototype("int version_get_minor_ert_version()")
    _micro_version = UtilPrototype("char* version_get_micro_ert_version()")
    _is_devel = UtilPrototype("bool version_is_ert_devel_version()")

    def __init__(self, major, minor, micro):
        self.major = major
        self.minor = minor
        self.micro = micro
        try:
            self.micro_int = int(micro)
            self.is_devel = False
        except ValueError:
            self.micro_int = -1
            self.is_devel = True

    def is_devel_version(self):
        return self.is_devel

    def version_string(self):
        return "%d.%d.%s" % (self.major, self.minor, self.micro)

    def version_tuple(self):
        return self.major, self.minor, self.micro

    def __cmp_tuple(self):
        return self.major, self.minor, self.micro_int

    def __str__(self):
        return self.versionString()

    def __repr__(self):
        major, minor, micro = self.versionTuple()
        commit = self._git_commit_short()
        status = 'production'
        if self.isDevelVersion():
            status = 'development'
        fmt = 'Version(major=%d, minor=%d, micro="%s", commit="%s", status="%s")'
        return fmt % (major, minor, micro, commit, status)

    @cmp_method
    def __eq__(self, other):
        return self.versionTuple() == other.versionTuple()

    def __ne__(self, other):
        return not (self == other)

    def __hash__(self):
        return hash(self.versionTuple())

    # All development versions are compared with micro version == -1;
    # i.e.  the two versions Version(1,2,"Alpha") and
    # Version(1,2,"Beta") compare as equal in the >= and <= tests -
    # but not in the == test.

    @cmp_method
    def __ge__(self, other):
        return self.__cmp_tuple() >= other.__cmp_tuple()

    @cmp_method
    def __lt__(self, other):
        return not (self >= other)

    @cmp_method
    def __le__(self, other):
        return self.__cmp_tuple() <= other.__cmp_tuple()

    @cmp_method
    def __gt__(self, other):
        return not (self <= other)

    @classmethod
    def get_build_time(cls):
        return cls._build_time()

    @classmethod
    def get_git_commit(cls, short=False):
        if not short:
            return cls._git_commit()
        else:
            return cls._git_commit_short()

    @classmethod
    def current_version(cls):
        major = cls._major_version()
        minor = cls._minor_version()
        micro = cls._micro_version()
        return Version(major, minor, micro)

    @classmethod
    def get_version(cls):
        v = cls.currentVersion()
        return v.versionString()

monkey_the_camel(Version, 'isDevelVersion', Version.is_devel_version)
monkey_the_camel(Version, 'versionString', Version.version_string)
monkey_the_camel(Version, 'versionTuple', Version.version_tuple)
monkey_the_camel(Version, 'getBuildTime', Version.get_build_time, classmethod)
monkey_the_camel(Version, 'getGitCommit', Version.get_git_commit, classmethod)
monkey_the_camel(Version, 'currentVersion', Version.current_version, classmethod)
monkey_the_camel(Version, 'getVersion', Version.get_version, classmethod)
