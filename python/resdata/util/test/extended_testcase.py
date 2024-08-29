import numbers
import os
import os.path
import traceback
import sys

from unittest import TestCase

from .source_enumerator import SourceEnumerator
from resdata.util.util import installAbortSignals
from resdata.util.util import Version


# Function wrapper which can be used to add decorator @log_test to test
# methods. When a test has been decorated with @log_test it will print
# "starting: <test_name>" when a method is complete and "complete: <test_name>"
# when the method is complete. Convenient when debugging tests which fail hard
# or lock up.


def log_test(test):
    def wrapper(*args):
        sys.stderr.write(f"starting: {test.__name__} \n")
        test(*args)
        sys.stderr.write(f"complete: {test.__name__} \n")

    return wrapper


class _AssertNotRaisesContext:
    def __init__(self, test_class):
        super().__init__()
        self._test_class = test_class

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_value, tb):
        if exc_type is not None:
            try:
                exc_name = exc_type.__name__
            except AttributeError:
                exc_name = str(exc_type)
            self._test_class.fail(
                f"Exception: {exc_name} raised\n{traceback.print_exception(exc_type, exc_value, tb)}"
            )
        return True


"""
This class provides some extra functionality for testing values that are almost equal.
"""


class ExtendedTestCase(TestCase):
    TESTDATA_ROOT = None
    SHARE_ROOT = None
    SOURCE_ROOT = None

    def __init__(self, *args, **kwargs):
        installAbortSignals()
        super().__init__(*args, **kwargs)

    def assertFloatEqual(self, first, second, msg=None, tolerance=1e-6):
        try:
            f_first, f_second = float(first), float(second)
            diff = abs(f_first - f_second)
            scale = max(1, abs(first) + abs(second))
            if msg is None:
                msg = f"Floats not equal: |{f_first:f} - {f_second:f}| > {tolerance:g}"
            self.assertTrue(diff < tolerance * scale, msg=msg)
        except TypeError:
            self.fail(
                f"Cannot compare as floats: {first} ({type(first)}) and {second} ({type(second)})"
            )

    def assertAlmostEqualList(self, first, second, msg=None, tolerance=1e-6):
        if len(first) != len(second):
            self.fail("Lists are not of same length!")

        for index in range(len(first)):
            self.assertFloatEqual(
                first[index], second[index], msg=msg, tolerance=tolerance
            )

    def assertImportable(self, module_name):
        try:
            __import__(module_name)
        except ImportError:
            tb = traceback.format_exc()
            self.fail(f"Module {module_name} not found!\n\nTrace:\n{str(tb)}")
        except Exception:
            tb = traceback.format_exc()
            self.fail(
                f"Import of module {module_name} caused errors!\n\nTrace:\n{str(tb)}"
            )

    def assertFilesAreEqual(self, first, second):
        if not self.__filesAreEqual(first, second):
            self.fail("Buffer contents of files are not identical!")

    def assertFilesAreNotEqual(self, first, second):
        if self.__filesAreEqual(first, second):
            self.fail("Buffer contents of files are identical!")

    def assertFileExists(self, path):
        if not os.path.exists(path) or not os.path.isfile(path):
            self.fail(f"The file: {path} does not exist!")

    def assertDirectoryExists(self, path):
        if not os.path.exists(path) or not os.path.isdir(path):
            self.fail(f"The directory: {path} does not exist!")

    def assertFileDoesNotExist(self, path):
        if os.path.exists(path) and os.path.isfile(path):
            self.fail(f"The file: {path} exists!")

    def assertDirectoryDoesNotExist(self, path):
        if os.path.exists(path) and os.path.isdir(path):
            self.fail(f"The directory: {path} exists!")

    def __filesAreEqual(self, first, second):
        buffer1 = open(first, "rb").read()  # noqa: SIM115
        buffer2 = open(second, "rb").read()  # noqa: SIM115

        return buffer1 == buffer2

    def assertEnumIsFullyDefined(
        self, enum_class, enum_name, source_path, verbose=False
    ):
        if self.SOURCE_ROOT is None:
            raise Exception("SOURCE_ROOT is not set.")

        enum_values = SourceEnumerator.findEnumerators(
            enum_name, os.path.join(self.SOURCE_ROOT, source_path)
        )

        for identifier, value in enum_values:
            if verbose:
                print("%s = %d" % (identifier, value))

            self.assertTrue(
                identifier in enum_class.__dict__,
                f"Enum does not have identifier: {identifier}",
            )
            class_value = enum_class.__dict__[identifier]
            self.assertEqual(
                class_value,
                value,
                f"Enum value for identifier: {identifier} does not match: {class_value} != {value}",
            )

    @classmethod
    def createSharePath(cls, path):
        if cls.SHARE_ROOT is None:
            raise Exception(
                "Trying to create directory rooted in 'SHARE_ROOT' - variable 'SHARE_ROOT' is not set."
            )
        return os.path.realpath(os.path.join(cls.SHARE_ROOT, path))

    @classmethod
    def createTestPath(cls, path):
        if cls.TESTDATA_ROOT is None:
            raise Exception(
                "Trying to create directory rooted in 'TESTDATA_ROOT' - variable 'TESTDATA_ROOT' has not been set."
            )
        return os.path.realpath(os.path.join(cls.TESTDATA_ROOT, path))

    def assertNotRaises(self, func=None):
        context = _AssertNotRaisesContext(self)
        if func is None:
            return context

        with context:
            func()

    @staticmethod
    def slowTestShouldNotRun():
        """
        @param: The slow test flag can be set by environment variable SKIP_SLOW_TESTS = [True|False]
        """

        return os.environ.get("SKIP_SLOW_TESTS", "False") == "True"

    @staticmethod
    def requireVersion(major, minor, micro="git"):
        required_version = Version(major, minor, micro)
        current_version = Version.currentVersion()

        return required_version < current_version
