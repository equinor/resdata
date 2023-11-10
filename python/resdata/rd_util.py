"""
Constants from the header rd_util.h - some stateless functions.

This module does not contain any class definitions; it mostly consists
of enum definitions/values from rd_util.h; the enum values are
extracted from the shared library in a semi-automagic manner using the
BaseCEnum class from cwrap.

In addition to the enum definitions there are a few stateless
functions from rd_util.c which are not bound to any class type.
"""
from __future__ import absolute_import

import ctypes

from cwrap import BaseCEnum
from resdata import ResdataPrototype
from resdata.util.util import monkey_the_camel


class FileType(BaseCEnum):
    TYPE_NAME = "rd_file_enum"
    OTHER = None
    RESTART = None
    UNIFIED_RESTART = None
    SUMMARY = None
    UNIFIED_SUMMARY = None
    SUMMARY_HEADER = None
    GRID = None
    EGRID = None
    INIT = None
    RFT = None
    DATA = None


FileType.addEnum("OTHER", 0)
FileType.addEnum("RESTART", 1)
FileType.addEnum("UNIFIED_RESTART", 2)
FileType.addEnum("SUMMARY", 4)
FileType.addEnum("UNIFIED_SUMMARY", 8)
FileType.addEnum("SUMMARY_HEADER", 16)
FileType.addEnum("GRID", 32)
FileType.addEnum("EGRID", 64)
FileType.addEnum("INIT", 128)
FileType.addEnum("RFT", 256)
FileType.addEnum("DATA", 512)


# -----------------------------------------------------------------


class Phase(BaseCEnum):
    TYPE_NAME = "rd_phase_enum"
    OIL = None
    GAS = None
    WATER = None


Phase.addEnum("OIL", 1)
Phase.addEnum("GAS", 2)
Phase.addEnum("WATER", 4)


# -----------------------------------------------------------------


class UnitSystem(BaseCEnum):
    TYPE_NAME = "rd_unit_enum"

    METRIC = None
    FIELD = None
    LAB = None
    PVT_M = None


UnitSystem.addEnum("METRIC", 1)
UnitSystem.addEnum("FIELD", 2)
UnitSystem.addEnum("LAB", 3)
UnitSystem.addEnum("PVT_M", 4)


# -----------------------------------------------------------------


class FileMode(BaseCEnum):
    TYPE_NAME = "rd_file_flag_enum"
    DEFAULT = None
    CLOSE_STREAM = None
    WRITABLE = None


FileMode.addEnum("DEFAULT", 0)
FileMode.addEnum("CLOSE_STREAM", 1)
FileMode.addEnum("WRITABLE", 2)


# -----------------------------------------------------------------


class ResdataUtil(object):
    _get_num_cpu = ResdataPrototype("int rd_get_num_cpu(char*)", bind=False)
    _get_file_type = ResdataPrototype(
        "rd_file_enum rd_get_file_type(char*, bool*, int*)", bind=False
    )
    _get_start_date = ResdataPrototype("rd_time_t rd_get_start_date(char*)", bind=False)
    _get_report_step = ResdataPrototype("int rd_filename_report_nr(char*)", bind=False)

    @staticmethod
    def get_num_cpu(datafile):
        """
        Parse ECLIPSE datafile and determine how many CPUs are needed.

        Will look for the "PARALLELL" keyword, and then read off the
        number of CPUs required. Will return one if no PARALLELL keyword
        is found.
        """
        return ResdataUtil._get_num_cpu(datafile)

    @staticmethod
    def get_file_type(filename):
        """
        Will inspect an ECLIPSE filename and return an integer type flag.
        """
        file_type, fmt, step = ResdataUtil.inspectExtension(filename)
        return file_type

    @staticmethod
    def get_start_date(datafile):
        return ResdataUtil._get_start_date(datafile).datetime()

    @staticmethod
    def inspect_extension(filename):
        """Will inspect a filename and return a tuple consisting of
        a FileType, a bool for formatted or not, and an
        integer for the step number.
        """
        fmt_file = ctypes.c_bool()
        report_step = ctypes.c_int(-1)
        file_type = ResdataUtil._get_file_type(
            filename, ctypes.byref(fmt_file), ctypes.byref(report_step)
        )
        if report_step.value == -1:
            step = None
        else:
            step = report_step.value

        return (file_type, fmt_file.value, step)

    @staticmethod
    def report_step(filename):
        report_step = ResdataUtil._get_report_step(filename)
        if report_step < 0:
            raise ValueError("Could not infer report step from: %s" % filename)

        return report_step


get_num_cpu = ResdataUtil.get_num_cpu
get_file_type = ResdataUtil.get_file_type
get_start_date = ResdataUtil.get_start_date

monkey_the_camel(
    ResdataUtil, "inspectExtension", ResdataUtil.inspect_extension, staticmethod
)
monkey_the_camel(ResdataUtil, "reportStep", ResdataUtil.report_step, staticmethod)
